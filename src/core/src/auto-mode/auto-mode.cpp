#include "auto-mode.hpp"
#include "common/program-phases.hpp"
#include "common/program.hpp"
#include "common/load-axis-list.hpp"
#include "eng/sibus/sibus.hpp"

#include <eng/log.hpp>
#include <eng/base64.hpp>
#include <eng/timer.hpp>
#include <eng/json.hpp>

#include <algorithm>
#include <chrono>

template <typename T>
static constexpr std::string to_string_axis_position(T const &list)
{
    std::stringstream ss;
    ss << "[";
    std::ranges::for_each(list, [&ss](auto const &pair) {
        ss << std::format("[{} = {:.3f}]", pair.first, pair.second);
    });
    ss << "]";

    return ss.str();
}

auto_mode::auto_mode()
    : eng::sibus::node("auto-mode")
    , vm_(phases_, target_axis_)
    , isc_(this)
{
    ictl_ = node::add_input_wire();
    node::set_activate_handler(ictl_, [this](eng::abc::pack) { activate(); });
    node::set_deactivate_handler(ictl_, [this] { deactivate(); });

    // Управление движением
    axis_ctl_ = node::add_output_wire("axis");
    node::set_wire_status_handler(axis_ctl_, [this]
    {
        eng::log::info("{}[axis-ctl]: -> {}", name(), eng::sibus::to_string(node::status(axis_ctl_)));
        (this->*sstate_)(0, node::status(axis_ctl_));
    });

    // Управление движением
    stuff_ctl_ = node::add_output_wire("stuff");
    node::set_wire_status_handler(stuff_ctl_, [this]
    {
        eng::log::info("{}[stuff-ctl]: -> {}", name(), eng::sibus::to_string(node::status(stuff_ctl_)));
        (this->*sstate_)(1, node::status(stuff_ctl_));
    });

    node::add_input_port_unsafe("program", [this](eng::abc::pack args)
    {
        upload_program(std::move(args));

        if (sstate_ == &auto_mode::ss_system_ready || sstate_ == &auto_mode::ss_wait_system_ready)
            (this->*sstate_)(1, node::status(stuff_ctl_));
    });

    phase_id_out_ = node::add_output_port("phase-id");
    times_out_ = node::add_output_port("times");

    load_axis_list();

    // Таймер, запускающий продолжение выполнения программы
    pause_timer_ = eng::timer::create([this]
    {
        eng::timer::stop(pause_timer_);
        isc_.switch_to_state(&auto_mode::s_pause_done);
    });

    // Таймер выполнения режима
    proc_timer_ = eng::timer::create([this]
    {
        update_output_times();
    });

    sstate_ = &auto_mode::ss_wait_system_ready;

    // Связываем свои входные и выходные провода
    // node::link_wires(ictl_, axis_ctl_);
    // node::link_wires(ictl_, stuff_ctl_);
}

void auto_mode::ss_wait_system_ready(std::size_t idx, eng::sibus::istatus status)
{
    output_.set(idx, status == eng::sibus::istatus::ready);
    if (!output_.all() || program_b64_.empty()) return;
    sstate_ = &auto_mode::ss_system_ready;
    isc_.switch_to_state(nullptr);
    node::ready(ictl_);
}

void auto_mode::ss_system_ready(std::size_t idx, eng::sibus::istatus status)
{
    output_.set(idx, status == eng::sibus::istatus::ready);
    if (output_.all() && !program_b64_.empty()) return;
    sstate_ = &auto_mode::ss_wait_system_ready;
    node::terminate(ictl_, "Система не готова");
}

void auto_mode::ss_system_in_proc(std::size_t idx, eng::sibus::istatus status)
{
    constexpr static eng::sibus::istatus istatus[] {
        eng::sibus::istatus::ready, eng::sibus::istatus::active
    };

    output_.set(idx, status == istatus[idx]);
    if (output_.all())
    {
        isc_.switch_to_state(&auto_mode::s_initialize);
        return;
    }
    sstate_ = &auto_mode::ss_wait_system_ready;
    isc_.switch_to_state(nullptr);

    node::deactivate(axis_ctl_);
    node::deactivate(stuff_ctl_);

    node::terminate(ictl_, "Целостность состояния cистемы нарушена");
}

void auto_mode::ss_wait_moving_start(std::size_t idx, eng::sibus::istatus status)
{
    constexpr static eng::sibus::istatus istatus[] {
        eng::sibus::istatus::active, eng::sibus::istatus::active
    };

    output_.set(idx, status == istatus[idx]);
    if (output_.all())
    {
        sstate_ = &auto_mode::ss_wait_moving_done;
        return;
    }

    sstate_ = &auto_mode::ss_wait_system_ready;
    isc_.switch_to_state(nullptr);

    node::deactivate(axis_ctl_);
    node::deactivate(stuff_ctl_);

    node::terminate(ictl_, "Целостность состояния cистемы нарушена");
}

void auto_mode::ss_wait_moving_done(std::size_t idx, eng::sibus::istatus status)
{
    constexpr static eng::sibus::istatus istatus[] {
        eng::sibus::istatus::ready, eng::sibus::istatus::active
    };

    output_.set(idx, status == istatus[idx]);
    if (output_.all())
    {
        sstate_ = &auto_mode::ss_wait_moving_done;
        isc_.switch_to_state(&auto_mode::s_moving_done);
        return;
    }

    sstate_ = &auto_mode::ss_wait_system_ready;
    isc_.switch_to_state(nullptr);

    node::deactivate(axis_ctl_);
    node::deactivate(stuff_ctl_);

    node::terminate(ictl_, "Целостность состояния cистемы нарушена");
}

void auto_mode::activate()
{
    eng::log::info("{}: {}", name(), __func__);

    if (isc_.is_in_state(&auto_mode::s_infinity_pause))
    {
        eng::timer::stop(pause_timer_);
        isc_.switch_to_state(&auto_mode::s_pause_done);
        return;
    }

    if (!isc_.is_in_state(nullptr))
    {
        node::reject(ictl_, "Программа уже выполняется");
        return;
    }

    // Программа не задана
    if (program_b64_.empty())
    {
        eng::log::error("{}: Необходимо задать программу", name());
        node::reject(ictl_, "Необходимо задать программу");
        return;
    }

    // Готовим программу к исполнению
    bool use_fc{ false };
    std::array<bool, 3> use_sp { false };
    std::vector<char> use_spin_axis;
    if (!prepare_program(use_fc, use_sp, use_spin_axis))
    {
        eng::log::error("{}: Программа содержит ошибки", name());
        node::reject(ictl_, "Программа содержит ошибки");
        return;
    }

    // Формируем требования к системе
    eng::abc::pack args{ use_fc };
    for (std::size_t i = 0; i < use_sp.size(); ++i)
        args.push(use_sp[i]);
    args.push<std::uint8_t>(use_spin_axis.size());
    std::ranges::for_each(use_spin_axis, [&args](char axis)
        { args.push(axis); });

    // Запускаем в работу узел управляения устройствами
    node::activate(stuff_ctl_, std::move(args));

    sstate_ = &auto_mode::ss_system_in_proc;
}

void auto_mode::deactivate()
{
    eng::log::info("{}: {}", name(), __func__);

    eng::timer::stop(pause_timer_);
    eng::timer::stop(proc_timer_);

    update_output_times();

    if (node::is_ready(axis_ctl_) && node::is_ready(stuff_ctl_))
    {
        isc_.switch_to_state(nullptr);
        return;
    }

    node::deactivate(axis_ctl_);
    node::deactivate(stuff_ctl_);

    sstate_ = &auto_mode::ss_wait_system_ready;

    node::terminate(ictl_, "Выполнение программы остановлено");
}

void auto_mode::s_initialize()
{
    eng::log::info("{}: {}", name(), __func__);

    eng::timer::start(proc_timer_, std::chrono::milliseconds(200));
    stopwatch_.restart();

    // Инициализируем наш процессор
    vm_.initialize();

    // Сохраняем текущую позицию как точку старта программы
    axis_initial_pos_ = axis_real_pos_;

    // Позиция программы равна 0.0
    std::ranges::for_each(axis_real_pos_, [this](auto const &pair) {
        axis_program_pos_[pair.first] = 0.0;
    });

    isc_.switch_to_state(&auto_mode::s_program_execution_loop);
}

void auto_mode::s_init_pause()
{
    eng::log::info("{}: {}", name(), __func__);

    // Запускаем счетчик времени
    pause_stopwatch_.restart();

    auto msec = vm_.pause_timeout_ms();
    if (msec != 0)
        eng::timer::start(pause_timer_, std::chrono::milliseconds(msec));
    pause_timeout_ = (msec != 0) ? (msec / 1000.0) : NAN;

    isc_.switch_to_state(msec ? &auto_mode::s_pause : &auto_mode::s_infinity_pause);

    update_output_times();
}

// Контролируем доступность требуемых нам для работы модулей
void auto_mode::s_pause()
{
    eng::log::info("{}: {}", name(), __func__);
}

// Контролируем доступность требуемых нам для работы модулей
void auto_mode::s_infinity_pause()
{
    eng::log::info("{}: {}", name(), __func__);
}

void auto_mode::s_pause_done()
{
    eng::log::info("{}: {}", name(), __func__);

    vm_.to_next_phase();
    isc_.switch_to_state(&auto_mode::s_program_execution_loop);
}

void auto_mode::s_execute_operation()
{
    eng::log::info("{}: {}", name(), __func__);

    execute_operation();
    vm_.to_next_phase();

    isc_.switch_to_state(vm_.has_phase_ops() ?
        &auto_mode::s_execute_operation : &auto_mode::s_program_execution_loop);
}

void auto_mode::s_start_moving()
{
    eng::log::info("{}: {}", name(), __func__);

    // Формируем описание задачи для узла движения
    eng::abc::pack args;
    vm_.fill_cnc_task(args);

    node::activate(axis_ctl_, std::move(args));

    sstate_ = &auto_mode::ss_wait_moving_start;

    execute_operation();
}

void auto_mode::s_moving_done()
{
    eng::log::info("{}: {}", name(), __func__);

    vm_.to_next_phase();
    isc_.switch_to_state(&auto_mode::s_program_execution_loop);
}

void auto_mode::s_program_execution_loop()
{
    eng::log::info("{}: {}", name(), __func__);

    // Программа продолжит анализ с данного этапа
    std::uint32_t phase_id = vm_.to_next_phase();
    node::set_port_value(phase_id_out_, { phase_id, true });

    // Задач движения на данном этапе нету
    if (!vm_.create_continuous_moving_list())
    {
        // Программа штатно завершила выполнение
        if (vm_.is_program_done())
        {
            eng::log::info("{}: Программа штатно завершила выполнение", name());

            node::set_port_value(phase_id_out_, { });

            deactivate();

            return;
        }

        switch(vm_.phase_type())
        {
        case VmPhaseType::Operation:
            isc_.switch_to_state(&auto_mode::s_execute_operation);
            break;
        case VmPhaseType::Pause:
            isc_.switch_to_state(&auto_mode::s_init_pause);
            break;
        case VmPhaseType::GoTo:
            isc_.switch_to_state(&auto_mode::s_program_execution_loop);
            break;

        default:
            eng::log::error("{}: Программа на необрабатываемом этапе", name());
            deactivate();
            break;
        }

        return;
    }

    if (vm_.op_phase_type() == VmPhaseType::GoTo)
    {
        phase_id = vm_.op_phase_id();
        node::set_port_value(phase_id_out_, { phase_id, true });
    }

    isc_.switch_to_state(&auto_mode::s_start_moving);
}

void auto_mode::execute_operation()
{
    eng::abc::pack args;
    vm_.fill_stuff_task(vm_.op_phase_id(), args);
    node::send_wire_signal(stuff_ctl_, std::move(args));

    // std::println();
    // eng::log::info("DO PHASE: {}", vm_.op_phase_id());
    // eng::log::info("{}\n", to_string_axis_position(axis_program_pos_));
}

void auto_mode::process_axis_positions()
{
    if (!isc_.is_in_state(&auto_mode::s_start_moving))
        return;

    // Отслеживаем где мы находимся позиционно
    // инкрементируя номер выполняемой строчки программы
    if (!vm_.check_in_position(axis_program_pos_))
        return;

    eng::log::info("{}: {}", name(), __func__);

    vm_.to_next_phase();
    execute_operation();

    // Тут у нас всегда есть следующий этап потому как
    // check_in_position не обрабатывает последний этап
    std::uint32_t phase_id = vm_.op_phase_id();
    node::set_port_value(phase_id_out_, { phase_id, true });
}

void auto_mode::upload_program(eng::abc::pack args)
{
    program_b64_ = args ? eng::abc::get_sv(args) : "";
    node::wire_response(ictl_, true, { });
    eng::log::info("{}: upload_program: {}", name(), program_b64_);
}

bool auto_mode::prepare_program(bool &use_fc, std::array<bool, 3> &use_sp, std::vector<char> &use_spin_axis)
{
    eng::log::info("{}: auto_mode::prepare_program", name());

    // Формируем список этапов
    eng::buffer::id_t mem { eng::base64::decode(program_b64_) };

    program p{};
    auto data = eng::buffer::get_content_region(mem);
    p.load(data);

    target_axis_.clear();
    p.modify_using_axis(use_spin_axis, target_axis_);

    phases_.clear();

    // Формируем из программы список этапов для выполнения
    p.for_each([&](program::op_type type, std::size_t rid)
    {
        switch(type)
        {
        case program::op_type::main:
            create_operation(p, rid, use_fc, use_sp);
            break;
        case program::op_type::pause:
            phases_.push_back(new VmPause(p.pause_ops[rid].msec));
            break;
        case program::op_type::loop: {
            auto const& op = p.loop_ops[rid];
            phases_.push_back(new VmGoTo(op.opid, op.N));
            break; }
        case program::op_type::fc: {
            use_fc = true;
            auto const& op = p.fc_ops[rid];
            phases_.push_back(new VmTimedFC(op.p, op.i, op.tv));
            break; }
        case program::op_type::center: {
            auto const& op = p.center_ops[rid];
            if (op.type == centering_type::shaft)
                phases_.push_back(new VmCenterShaft());
            else
            {
                int dir = (op.type == centering_type::tooth_in) ? 1 : -1;
                phases_.push_back(new VmCenterTooth(dir, op.shift));
            }
            break; }
        }
    });

    eng::log::info("{}: Программа сформирована, этапов {}", name(), phases_.size());

    return true;
}

void auto_mode::create_operation(program const& p, std::size_t rid, bool &use_fc, std::array<bool, 3> &use_sp) noexcept
{
    auto const& op = p.main_ops[rid];
    VmOperation *pop = new VmOperation(op.absolute);

    p.for_fc(rid, [&](std::size_t id, program::fc_item_t const& fc) 
    {
        use_fc |= (std::fpclassify(fc.P) != FP_ZERO);
        pop->items.push_back(new VmOperation::FC(id, fc.I, fc.P));
    });

    p.for_sprayer(rid, [&](std::size_t id, bool state) 
    {
        use_sp[id] |= state;
        pop->items.push_back(new VmOperation::Sprayer(id, state));
    });

    p.for_spin_axis(rid, [pop](char axis, float speed) 
    {
        pop->items.push_back(new VmOperation::Spin(axis, speed));
    });

    p.for_target_axis(rid, [pop](char axis, float pos) 
    {
        pop->items.push_back(new VmOperation::Pos(axis, pos));
    });

    pop->items.push_back(new VmOperation::Speed(op.target.speed));

    phases_.push_back(pop);
}

void auto_mode::load_axis_list()
{
    ambit::load_axis_list([this](char axis, std::string_view, bool)
    {
        axis_real_pos_[axis] = 0.0;

        node::add_input_port(std::format("{}-position", axis), [this, axis](eng::abc::pack args)
        {
            double position = eng::abc::get<double>(args);
            axis_real_pos_[axis] = position;

            axis_program_pos_[axis] = position - axis_initial_pos_[axis];

            process_axis_positions();
        });
    });
}

void auto_mode::update_output_times()
{
    // Общее время выполнения
    double t0 = stopwatch_.elapsed_seconds<double>();

    // Время на паузе или на бесконечной паузе
    double t1 = NAN;
    if (isc_.is_in_state(&auto_mode::s_pause))
        t1 = pause_timeout_ - pause_stopwatch_.elapsed_seconds<double>();
    else if (isc_.is_in_state(&auto_mode::s_infinity_pause))
        t1 = pause_stopwatch_.elapsed_seconds<double>();

    bool inf = isc_.is_in_state(&auto_mode::s_infinity_pause);

    node::set_port_value(times_out_, { t0, t1, inf });
}


