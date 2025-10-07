#include "drainage-ctl.h"

#include <hardware.h>
#include <aem/log.h>

drainage_ctl::drainage_ctl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , hw_(hw)
    , H7_ctl_(hw.get_unit_by_class(sid, "H7"), "H7")
{ 
    hw.LSET.add(sid, "min", [this](nlohmann::json const &value) 
    {
        if (value.get<bool>())
        {
            hw_.log_message(LogMsg::Warning, name(), "Уровень достиг минимума, выключаем насос");
            pump_ = false;
        }
    });
    
    hw.LSET.add(sid, "max", [this](nlohmann::json const &value) 
    {
        if (value.get<bool>())
        {
            hw_.log_message(LogMsg::Warning, name(), "Уровень достиг максимума, включаем насос");
            pump_ = true;
        }
    });
    
    hw.LSET.add(sid, "H7", [this](nlohmann::json const &value) 
    {
        hw_.log_message(LogMsg::Info, name(), 
            fmt::format("Насос H7 {}", value.get<bool>() ? "включен" : "выключен"));
    });
}

void drainage_ctl::on_activate() noexcept
{
    hw_.log_message(LogMsg::Info, name(), "Контроллер успешно активирован");
    
    pump_on_off_ = false;
    next_state(&drainage_ctl::turn_on_off_pump);
}

void drainage_ctl::on_deactivate() noexcept
{ 
    hw_.log_message(LogMsg::Info, name(), "Контроллер деактивирован");
}

void drainage_ctl::wait_state_changed()
{
    if (!pump_) return;
    
    pump_on_off_ = pump_.value();
    pump_.reset();
    
    next_state(&drainage_ctl::turn_on_off_pump);
}

void drainage_ctl::turn_on_off_pump()
{
    auto [ done, error ] = H7_ctl_.set(pump_on_off_);
    if (!dhresult::check(done, error, [this] { next_state(&drainage_ctl::turn_on_off_pump_error); }))
        return;
    // hw_.log_message(LogMsg::Info, name(), fmt::format("Насос {}", pump_on_off_ ? "включен" : "выключен"));
    next_state(&drainage_ctl::wait_state_changed);
}

void drainage_ctl::turn_on_off_pump_error()
{
    hw_.log_message(LogMsg::Error, name(), "Не удалось выполнить команду");
    next_state(&drainage_ctl::wait_state_changed);
}


// void drain_ctl::init_unit()
// {
//     if (init_step_ >= init_steps_.size())
//     {
//         next_state(&drain_ctl::wait_new_position);
//         return;
//     }
//     
//     auto const &step = init_steps_[init_step_];
//     auto [ done, error ] = step.driver->set(step.value);
//     if (!dhresult::check(done, error, [this] { next_state(&drain_ctl::init_unit_error); }))
//         return;
//     
//     hw_.log_message(LogMsg::Command, "drain", fmt::format("{}: {}", step.message, step.value));
//     init_step_ += 1;
//     
//     next_state(&drain_ctl::init_unit);
// }

// void drain_ctl::init_unit_error()
// {
//     auto const &step = init_steps_[init_step_];
//     
//     hw_.log_message(LogMsg::Error, "drain", fmt::format("{}", step.message));
//     
//     init_step_ += 1;
//     next_state(&drain_ctl::init_unit);
// }

// void drain_ctl::wait_new_position()
// {
//     if (!position_)
//         return;
//     target_pos_ = position_.value();
//     position_.reset();
//     
//     hw_.log_message(LogMsg::Command, "drain", fmt::format("Переключение в положение {}", target_pos_));
//     
//     next_state(&drain_ctl::do_target_pos);
// }

// void drain_ctl::do_target_pos()
// {
//     auto [ done, error ] = pos_ctl_.set(target_pos_);
//     if (!dhresult::check(done, error, [this] { next_state(&drain_ctl::do_target_pos_error); }))
//         return;
//     next_state(&drain_ctl::wait_new_position);
// }

// void drain_ctl::do_target_pos_error()
// {
//     hw_.log_message(LogMsg::Error, "drain", fmt::format("Не удалось переключить в положение {}", target_pos_));
//     target_pos_ = 0;
//     next_state(&drain_ctl::wait_new_position);
// }


// void drain_ctl::update_state(bool drain, bool tank) noexcept
// {
//     if (drain_ == drain && tank_ == tank)
//         return;
//     
//     drain_ = drain;
//     tank_ = tank;
//     
//     position_ = drain_ ? 3 : (tank_ ? 1 : 2);
// }

