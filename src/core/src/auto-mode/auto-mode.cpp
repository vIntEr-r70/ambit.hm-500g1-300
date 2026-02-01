#include "auto-mode.hpp"
#include "common/program.hpp"

#include <eng/log.hpp>
#include <eng/base64.hpp>
#include <eng/timer.hpp>
#include <eng/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>

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
{
    ictl_ = node::add_input_wire();
    node::set_activate_handler(ictl_, [this](eng::abc::pack) {
        activate();
    });
    node::set_deactivate_handler(ictl_, [this] {
        deactivate();
    });
    node::set_wire_request_handler(ictl_, [this](eng::abc::pack args) {
        handle_request(std::move(args));
    });

    // Управление движением
    axis_ctl_ = node::add_output_wire("axis");
    node::set_wire_status_handler(axis_ctl_, [this] {
        touch_current_state();
    });

    // Управление движением
    stuff_ctl_ = node::add_output_wire("stuff");
    node::set_wire_status_handler(stuff_ctl_, [this] {
        touch_current_state();
    });

    phase_id_out_ = node::add_output_port("phase-id");

    load_axis_list();

    // Таймер, запускающий продолжение выполнения программы
    pause_timer_ = eng::timer::create([this]
    {
        eng::timer::stop(pause_timer_);
        switch_to_state(&auto_mode::s_pause_done);
    });
}

// Запускаем программу на выполнение
bool auto_mode::activate()
{
    eng::log::info("auto_mode::activate");

    // Программа не задана
    if (program_b64_.empty())
    {
        eng::log::error("{}: Необходимо выбрать программу", name());
        node::deactivated(ictl_);
        return false;
    }

    // Проверяем наличие связи с требуемыми узлами
    bool axis_ready = node::is_ready(axis_ctl_);
    bool stuff_ready = node::is_ready(stuff_ctl_);

    // Система не готова к работе
    if (!axis_ready || !stuff_ready)
    {
        eng::log::error("{}: Система не может быть активирована", name());
        node::deactivated(ictl_);
        return false;
    }

    // Готовим программу к исполнению
    bool use_fc{ false };
    std::array<bool, 3> use_sp { false };
    std::vector<char> use_spin_axis;
    if (!prepare_program(use_fc, use_sp, use_spin_axis))
    {
        eng::log::error("{}: Программа содержит ошибки", name());
        node::deactivated(ictl_);
        return false;
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

    // Мы в состоянии инициализации
    switch_to_state(&auto_mode::s_initialize);

    return true;
}

// Прерываем выполнение программы
void auto_mode::deactivate()
{
    eng::log::info("{}: auto_mode::deactivate", name());
    switch_to_state(&auto_mode::s_deinitialize);
}

///////////////////////////////////////////////////////////////////////////

void auto_mode::s_initialize()
{
    eng::log::info("{}: {}", name(), __func__);

    if (node::is_transiting(stuff_ctl_))
        return;

    if (!node::is_active(stuff_ctl_))
    {
        eng::log::error("{}: Не удалось активировать систему управления периферией", name());
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }

    node::activated(ictl_);

    // Инициализируем наш процессор
    vm_.initialize();

    // Сохраняем текущую позицию как точку старта программы
    axis_initial_pos_ = axis_real_pos_;

    // Позиция программы равна 0.0
    std::ranges::for_each(axis_real_pos_, [this](auto const &pair) {
        axis_program_pos_[pair.first] = 0.0;
    });

    switch_to_state(&auto_mode::s_program_execution_loop);
}

void auto_mode::s_deinitialize()
{
    eng::log::info("{}: {}", name(), __func__);

    if (node::is_allow(axis_ctl_))
    {
        if (!node::is_transiting(axis_ctl_))
        {
            if (node::is_active(axis_ctl_))
                node::deactivate(axis_ctl_);
        }
    }

    if (node::is_allow(stuff_ctl_))
    {
        if (!node::is_transiting(stuff_ctl_))
        {
            if (node::is_active(stuff_ctl_))
                node::deactivate(stuff_ctl_);
        }
    }

    if (node::is_transiting(stuff_ctl_) || node::is_transiting(axis_ctl_))
        return;

    node::deactivated(ictl_);
    switch_to_state(nullptr);

    eng::log::info("{}: DEACTIVATED", name());
}

void auto_mode::s_init_pause()
{
    // eng::log::info("{}: {}", name(), __func__);

    auto msec = vm_.pause_timeout_ms();
    if (msec != 0)
        eng::timer::start(pause_timer_, std::chrono::milliseconds(msec));

    switch_to_state(msec ? &auto_mode::s_pause : &auto_mode::s_infinity_pause);
}

// Контролируем доступность требуемых нам для работы модулей
void auto_mode::s_pause()
{
    // eng::log::info("{}: {}", name(), __func__);

    if (!node::is_ready(axis_ctl_))
    {
        eng::log::error("{}: Система управления движения отвалилась", name());
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }

    if (!node::is_active(stuff_ctl_))
    {
        eng::log::error("{}: Система управления периферией отвалилась", name());
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }
}

// Контролируем доступность требуемых нам для работы модулей
void auto_mode::s_infinity_pause()
{
    eng::log::info("{}: {}", name(), __func__);

    if (!node::is_ready(axis_ctl_))
    {
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }

    if (!node::is_active(stuff_ctl_))
    {
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }
}

void auto_mode::s_pause_done()
{
    eng::log::info("{}: {}", name(), __func__);

    if (!node::is_ready(axis_ctl_))
    {
        eng::log::error("{}: Система управления движения отвалилась", name());
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }

    if (!node::is_active(stuff_ctl_))
    {
        eng::log::error("{}: Система управления периферией отвалилась", name());
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }

    vm_.to_next_phase();
    switch_to_state(&auto_mode::s_program_execution_loop);
}

void auto_mode::s_execute_operation()
{
    eng::log::info("{}: {}", name(), __func__);

    execute_operation();
    vm_.to_next_phase();

    switch_to_state(vm_.has_phase_ops() ?
        &auto_mode::s_execute_operation : &auto_mode::s_program_execution_loop);
}

void auto_mode::s_start_moving()
{
    eng::log::info("{}: {}", name(), __func__);

    // Формируем описание задачи для узла движения
    eng::abc::pack args;
    vm_.fill_cnc_task(args);

    node::activate(axis_ctl_, std::move(args));

    execute_operation();

    // local_segment_id_ = 0;
    switch_to_state(&auto_mode::s_wait_moving_done);

// #ifndef BUILDROOT
//     // Заглушка для режима отладки
//     eng::timer::once(std::chrono::seconds(3), [this] {
//         switch_to_state(&auto_mode::s_moving_done);
//     });
// #endif
}

void auto_mode::s_wait_moving_done()
{
    // eng::log::info("{}: {}", name(), __func__);

    if (!node::is_active(stuff_ctl_))
    {
        eng::log::error("{}: Система управления периферией отвалилась", name());
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }

    if (node::is_transiting(axis_ctl_))
        return;

    // Если движение все еще выполняется
    if (node::is_active(axis_ctl_))
    {
        // eng::log::info("{}: {}", name(), __func__);

        // Отслеживаем где мы находимся позиционно
        // инкрементируя номер выполняемой строчки программы
        if (!vm_.check_in_position(axis_program_pos_))
            return;

        vm_.to_next_phase();
        execute_operation();

        std::uint32_t phase_id = vm_.phase_id();
        node::set_port_value(phase_id_out_, { phase_id, true });

        return;
    }

    // Мы можем продолжать
    if (!node::is_ready(axis_ctl_))
    {
        eng::log::error("{}: Система управления движения отвалилась", name());
        switch_to_state(&auto_mode::s_deinitialize);
        return;
    }

    vm_.to_next_phase();
    switch_to_state(&auto_mode::s_program_execution_loop);
}

// void auto_mode::s_moving_done()
// {
//     eng::log::info("{}: {}", name(), __func__);
//     switch_to_state(&auto_mode::s_execute_operation);
//     // vm_.to_next_phase();
//     // switch_to_state(&auto_mode::s_program_execution_loop);
// }

void auto_mode::s_program_execution_loop()
{
    eng::log::info("{}: {}", name(), __func__);

    std::uint32_t phase_id = vm_.phase_id();
    node::set_port_value(phase_id_out_, { phase_id, true });

    // Задач движения на данном этапе нету
    if (!vm_.create_continuous_moving_list())
    {
        // Программа штатно завершила выполнение
        if (vm_.is_program_done())
        {
            eng::log::info("{}: Программа штатно завершила выполнение", name());

            std::uint32_t phase_id = vm_.phase_id();
            node::set_port_value(phase_id_out_, { phase_id, true });

            switch_to_state(&auto_mode::s_deinitialize);
            return;
        }

        switch(vm_.phase_type())
        {
        case VmPhaseType::Operation:
            switch_to_state(&auto_mode::s_execute_operation);
            break;
        case VmPhaseType::Pause:
            switch_to_state(&auto_mode::s_init_pause);
            break;

        default:
            eng::log::error("{}: Программа на необрабатываемом этапе", name());
            switch_to_state(&auto_mode::s_deinitialize);
            break;
        }

        return;
    }

    switch_to_state(&auto_mode::s_start_moving);

    // Имеем варианты дальнейшего развития событий
    // 1: нет ни одной задачи движения
    //  1.1: значит у нас либо закончилась программа
    //  1.2: либо операция не требует движения
    // 2: есть задачи движения и их требуется выполнить

    // Получаем список задачь движения и список этапов
}

void auto_mode::execute_operation()
{
    eng::abc::pack args;
    vm_.fill_stuff_task(vm_.phase_id(), args);
    node::send_wire_signal(stuff_ctl_, std::move(args));

    std::println();
    eng::log::info("DO PHASE: {}", vm_.phase_id());
    eng::log::info("{}\n", to_string_axis_position(axis_program_pos_));
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
        // eng::log::trace("create_operation: spin: {} = {}", axis, speed);
        pop->items.push_back(new VmOperation::Spin(axis, speed));
    });

    p.for_target_axis(rid, [pop](char axis, float pos) 
    {
        // eng::log::trace("create_operation: pos: {} = {}", axis, pos);
        pop->items.push_back(new VmOperation::Pos(axis, pos));
    });

    pop->items.push_back(new VmOperation::Speed(op.target.speed));

    phases_.push_back(pop);
}

///////////////////////////////////////////////////////////////////////////

// Запрос на управление
void auto_mode::handle_request(eng::abc::pack args)
{
    static std::unordered_map<std::string_view, command_handler> const map {
        { "upload-program", &auto_mode::cmd_upload_program  },
    };

    std::string_view cmd = eng::abc::get_sv(args);

    auto it = map.find(cmd);
    if (it == map.end())
    {
        node::wire_response(ictl_, false, { std::format("Незнакомая комманда: {}", cmd) });
        return;
    }

    args.pop_front();

    (this->*(it->second))(std::move(args));
}

void auto_mode::cmd_upload_program(eng::abc::pack args)
{
    if (is_in_state())
    {
        node::wire_response(ictl_, false, { "Программа выполняется" });
        return;
    }

    program_b64_ = eng::abc::get_sv(args);
    node::wire_response(ictl_, true, { });

    eng::log::info("auto_mode::cmd_upload_program: {}", program_b64_);
}

void auto_mode::register_on_bus_done()
{
    // return;

#ifndef BUILDROOT
    eng::log::info("auto_mode::register_on_bus_done");

    program_b64_ = "AQAAAAMAAAABAAAAVgQAAABYWVpWAwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUQAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABRAAAAAAAAANEAAAAAAAAAAAAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFEAAAAAAAAA0wAAAAAAAAAAAAAAAAAAANEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAAAAAAAA";

    program_b64_ = "AQAAAAMAAAABAAAAVgQAAABYWVpWBwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUQAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABRAAAAAAAAAOUAAAAAAAAAAAAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFEAAAAAAAAA0wAAAAAAAAAAAAAAAAAAANEAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUQAAAAAAAADTAAAAAAAAAAAAAAAAAAAA0QAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFEAAAAAAAAA0QAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAANAHAAAAAAAAAgAAAAAAAAADAAAAAAAAAAIAAAAAAAAAAAAAAAoAAAAAAAABAAACAAIA";

    program_b64_ = "AQAAAAMAAAABAAAAVgQAAABYWVpWBwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUQAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAABAQEAAAAAAAAAAAAAAAAAABRAAAAAAAAAOUAAAAAAAAAAAAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFEAAAAAAAAA0wAAAAAAAAAAAAAAAAAAANEAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAQEBAAAAAAAAAAAAAAAAAAAUQAAAAAAAADTAAAAAAAAAAAAAAAAAAAA0QAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAEBAQAAAAAAAAAAAAAAAAAAFEAAAAAAAAA0QAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAANAHAAAAAAAAAgAAAAAAAAADAAAAAAAAAAIAAAAAAAAAAAAAAAoAAAAAAAABAAACAAIA";

    program_b64_ = "AQAAAAMAAAABAAAAVgQAAABYWVpWAwAAAAEAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAUQAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAABRAAAAAAAAAOUAAAAAAAAAAAAAAAAAAADRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAFEAAAAAAAAA0wAAAAAAAAAAAAAAAAAAANEAAAAAAAAAAAAAAAAABAAAAAAAAAAEAAAAAAAAAAAAAAAQAAAAAAAAC";

    program_b64_ = "AQAAAAMAAAABAAAAVgQAAABYWVpWAQAAAAAAAAAAAAAAAAAAAAAAAAAAAQEBAAAAAAAAAAAAAAAAAAAUQAAAAAAAABRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAACgAAAAAAAAAAAAAAAgAAAAAC";

    eng::timer::once(std::chrono::milliseconds(100),
        [this] {
            activate();
        });

#endif
}

void auto_mode::load_axis_list()
{
    char const *LIAEM_RO_PATH = std::getenv("LIAEM_RO_PATH");
    if (LIAEM_RO_PATH == nullptr)
    {
        eng::log::error("{}: LIAEM_RO_PATH not set", name());
        return;
    }

    std::filesystem::path path(LIAEM_RO_PATH);
    path /= "axis.json";

    try
    {
        eng::json::array cfg(path);
        cfg.for_each([this](std::size_t, eng::json::value v)
        {
            eng::json::object obj(v);
            char axis = obj.get_sv("axis")[0];

            axis_real_pos_[axis] = 0.0;

            node::add_input_port(std::format("{}-position", axis), [this, axis](eng::abc::pack args)
            {
                double position = eng::abc::get<double>(args);
                axis_real_pos_[axis] = position;

                axis_program_pos_[axis] = position - axis_initial_pos_[axis];

                touch_current_state();
            });
        });
    }
    catch(std::exception const &e)
    {
        eng::log::error("{}: {}", name(), e.what());
        return;
    }
}


