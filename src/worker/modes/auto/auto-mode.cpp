#include "auto-mode.h"
#include "common/program.h"

#include <hardware.h>
#include <defs.h>

#include <aem/utils/base64.h>
#include <aem/log.h>

// Выполняем заданную программу

auto_mode::auto_mode(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("auto-mode")
    , hw_(hw)
    , vm_(hw, axis_cfg)
{ 
    hw_.SET.add("auto", "program", [this](nlohmann::json const &program) {
        program_b64_ = program.get_ref<std::string const&>();
    });
    
    hw_.START.add("auto", [this]
    {
        start_program();
    });
    hw_.STOP.add("auto", [this]
    {
        stop_program();
    });
    
    hw_.LSET.add("auto", "start", [this](nlohmann::json const &value) 
    {
        if (!value.get<bool>()) return;
        start_program();
    });
    
    hw_.LSET.add("auto", "stop", [this](nlohmann::json const &value) 
    {
        if (!value.get<bool>()) return;
        stop_program();
    });
    
    hw.LSET.add("auto", "STAT", [this](nlohmann::json const &value) 
    {
        if ((value.get<std::size_t>() == 2) && vm_.is_active())
            stop_program();
    });
    
    hw.listen_event("auto", "offline", [this](nlohmann::json const &value) 
    {
        if (value.get<bool>() && vm_.is_active())
            stop_program();
    });
}

bool auto_mode::is_ctrl_enabled() const noexcept 
{ 
    return vm_.is_in_infinity_pause();
}

void auto_mode::start_program() noexcept
{
    hw_.log_message(LogMsg::Command, "auto", "Команда на начало выполнения программы");
    
    // Программа не задана
    if (program_b64_.empty())
    {
        hw_.log_message(LogMsg::Error, "auto", "Необходимо выбрать программу");
        return;
    }

    // Станок в аварии
    if (hw_.is_locked_any())
    {
        hw_.log_message(LogMsg::Error, "auto", "Некоторый функционал станка заблокирован");
        return;
    }
    
    // Режим не активен
    if (!is_active())
    {
        hw_.log_message(LogMsg::Error, "auto", "Необходимо активировать автоматический режим");
        return;
    }
    
    // Если мы в режиме выполнения программы
    if (is_current_state(&auto_mode::program_in_proc))
    {
        // Если программа не на бесконечной паузе 
        if (!vm_.is_in_infinity_pause())
        {
            hw_.log_message(LogMsg::Error, "auto", "Программа уже запущена и не на бесконечной паузе");
            return;
        }
        
        // Продолжаем выполнение
        vm_.do_continue();

        return;
    }

    // Формируем список этапов
    aem::buffer mem = aem::utils::base64::decode(program_b64_);

    program p{};
    p.load(mem);

    spin_axis_.clear(); target_axis_.clear();
    p.modify_using_axis(spin_axis_, target_axis_);

    use_fc_ = false;
    use_sprayers_ = false;

    // Формируем из программы список этапов для выполнения
    p.for_each([this, &p](program::op_type type, std::size_t rid)
    {
        switch(type)
        {
        case program::op_type::main: 
            create_operation(p, rid);
            break;
        case program::op_type::pause:
            phases_.push_back(new VmPause(p.pause_ops[rid].msec));
            break;
        case program::op_type::loop: {
            auto const& op = p.loop_ops[rid];
            phases_.push_back(new VmGoTo(op.opid, op.N));
            break; }
        case program::op_type::fc: {
            use_fc_ |= true;
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

    aem::log::info("auto-mode::start_program: phases: {}", phases_.size());
}

void auto_mode::create_operation(program const& p, std::size_t rid) noexcept
{
    auto const& op = p.main_ops[rid];
    VmOperation *pop = new VmOperation(op.absolute);

    p.for_fc(rid, [this, pop](std::size_t id, program::fc_item_t const& fc) 
    {
        use_fc_ |= (std::fpclassify(fc.P) != FP_ZERO);
        pop->items.push_back(new VmOperation::FC(id, fc.I, fc.P));
    });

    p.for_sprayer(rid, [this, pop](std::size_t id, bool state) 
    {
        use_sprayers_ |= state;
        pop->items.push_back(new VmOperation::Sprayer(id, state));
    });

    p.for_spin_axis(rid, [pop](char axis, float speed) 
    {
        aem::log::trace("create_operation: spin: {} = {}", axis, speed);
        pop->items.push_back(new VmOperation::Spin(axis, speed));
    });

    p.for_target_axis(rid, [pop](char axis, float pos) 
    {
        aem::log::trace("create_operation: pos: {} = {}", axis, pos);
        pop->items.push_back(new VmOperation::Pos(axis, pos));
    });

    pop->items.push_back(new VmOperation::Speed(op.target.speed));

    phases_.push_back(pop);
}

void auto_mode::stop_program() noexcept
{
    hw_.log_message(LogMsg::Command, "auto", "Команда на останов выполнения программы");
    
    if (!vm_.is_active())
    {
        hw_.log_message(LogMsg::Error, "auto", "Программа не активна");
        return;
    }

    vm_.deactivate();
    next_state(&auto_mode::wait_deactivate);
}

// Загружаем программу из базы по ее идентификатору
void auto_mode::on_activate() noexcept
{
    hw_.nf_.notify({ "auto-mode", "pause" }, false);
    hw_.nf_.notify({ "auto-mode", "running" }, false);
    
    hw_.nf_.notify({ "auto-mode", "time-front" }, 0);
    next_state(&auto_mode::wait_program);
}

void auto_mode::on_deactivate() noexcept
{
    vm_.deactivate();
    next_state(&auto_mode::wait_deactivate);
}

void auto_mode::wait_deactivate() noexcept
{
    if (!vm_.is_done())
        return;
    
    hw_.nf_.notify({ "auto-mode", "running" }, false);
    hw_.nf_.notify({ "auto-mode", "pause" }, false);
    
    next_state(is_active() ? &auto_mode::wait_program : nullptr);
}

void auto_mode::wait_program() noexcept
{
    if (phases_.empty())
        return;

    if (!vm_.set_program(phases_, spin_axis_, target_axis_, use_fc_, use_sprayers_))
    {
        clear_phases();
        return;
    }

    // Включаем запись архива
    hw_.init_arc_write();

    clear_phases();

    hw_.log_message(LogMsg::Command, "auto", "Программа запущена");
    vm_.activate();

    hw_.nf_.notify({ "auto-mode", "time-front" }, 0);
    stopwatch_.restart();
    
    hw_.nf_.notify({ "auto-mode", "running" }, true);

    next_state(&auto_mode::program_in_proc);
}

void auto_mode::program_in_proc() noexcept
{
    if (!vm_.is_done())
    {
        hw_.nf_.notify({ "auto-mode", "time-front" }, stopwatch_.elapsed().seconds());
        return;
    }
    
    hw_.nf_.notify({ "auto-mode", "running" }, false);
    hw_.nf_.notify({ "auto-mode", "pause" }, false);

    // Финализируем архив 
    hw_.finalize_arc();

    hw_.log_message(LogMsg::Info, "auto", "Выполнение программы завершилось");

    aem::log::warn("Выполнение программы прекратилось!!!\n");

    // Ждем новую программу
    next_state(&auto_mode::wait_program);
}

void auto_mode::clear_phases() noexcept
{
    std::for_each(phases_.begin(), phases_.end(), [](auto &&phase) { delete phase; });
    phases_.clear();
}

