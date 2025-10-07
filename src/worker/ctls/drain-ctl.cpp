#include "drain-ctl.h"

#include <hardware.h>
#include <aem/log.h>

// Контроллируем положение 2х тумблеров
// Возможные положения первого: Бак/Канализация
// Возможные положения второго: Вода/Среда
// Тут мы управляем положением трубы:
// 1 - Вода
// 2 - Среда
// 3 - Канализация
// Наша задача определить требуемое положение и выставить его
drain_ctl::drain_ctl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , hw_(hw)
    , pos_ctl_(hw.get_unit_by_class(sid, "pos"), "pos")
    , timeout_ctl_(hw.get_unit_by_class(sid, "timeout"), "timeout")
    , VA10_ctl_(hw.get_unit_by_class(sid, "VA10"), "VA10")
    , VA11_ctl_(hw.get_unit_by_class(sid, "VA11"), "VA11")
{ 
    // Куда сливать
    hw.LSET.add(sid, "drain", [this](nlohmann::json const &value) {
        update_state(value.get<bool>(), tank_);
    });
    
    // Откуда сливать
    hw.LSET.add(sid, "tank", [this](nlohmann::json const &value) 
    {
        update_state(drain_, value.get<bool>());
    });
    
    hw.LSET.add(sid, "status", [this](nlohmann::json const &value)
    {
        std::uint32_t v = value.get<std::uint32_t>();
        if (status_ == v && status_ready_)
            return;
            
        status_ = v;
        status_ready_ = true;
        
        hw_.log_message(LogMsg::Info, name(), fmt::format("Статус: {}", status_));
    });
    
    hw.LSET.add(sid, "state", [this](nlohmann::json const &value)
    {
        std::uint16_t v = value.get<std::uint16_t>();
        if (state_ == v && state_ready_)
            return;
            
        state_ = v;
        state_ready_ = true;
        
        hw_.log_message(LogMsg::Info, name(), fmt::format("Состояние: {}", state_));
    });
    
    init_steps_.push_back({ &timeout_ctl_, 50, "Инициализация времени движения" });
}

void drain_ctl::on_activate() noexcept
{
    hw_.log_message(LogMsg::Info, name(), "Контроллер успешно активирован");
    
    init_step_ = 0;
    next_state(&drain_ctl::init_unit);
}

void drain_ctl::on_deactivate() noexcept
{ 
    hw_.log_message(LogMsg::Info, name(), "Контроллер деактивирован");
}

void drain_ctl::init_unit()
{
    if (init_step_ >= init_steps_.size())
    {
        next_state(&drain_ctl::wait_new_position);
        return;
    }
    
    auto const &step = init_steps_[init_step_];
    auto [ done, error ] = step.driver->set(step.value);
    if (!dhresult::check(done, error, [this] { next_state(&drain_ctl::init_unit_error); }))
        return;
    
    hw_.log_message(LogMsg::Command, name(), fmt::format("{}: {}", step.message, step.value));
    init_step_ += 1;
    
    next_state(&drain_ctl::init_unit);
}

void drain_ctl::init_unit_error()
{
    auto const &step = init_steps_[init_step_];
    
    hw_.log_message(LogMsg::Error, name(), fmt::format("{}", step.message));
    
    init_step_ += 1;
    next_state(&drain_ctl::init_unit);
}

void drain_ctl::wait_new_position()
{
    if (!position_)
        return;
    target_pos_ = position_.value();
    position_.reset();
    
    hw_.log_message(LogMsg::Command, name(), fmt::format("Переключение в положение {}", target_pos_));
    
    next_state(&drain_ctl::do_target_pos);
}

void drain_ctl::do_target_pos()
{
    auto [ done, error ] = pos_ctl_.set(target_pos_);
    if (!dhresult::check(done, error, [this] { next_state(&drain_ctl::do_target_pos_error); }))
        return;
    next_state(&drain_ctl::do_VA10);
}

void drain_ctl::do_VA10()
{
    auto [ done, error ] = VA10_ctl_.set(target_pos_ == 1);
    if (!dhresult::check(done, error, [this] { next_state(&drain_ctl::do_target_pos_error); }))
        return;
    next_state(&drain_ctl::do_VA11);
}

void drain_ctl::do_VA11()
{
    auto [ done, error ] = VA11_ctl_.set(target_pos_ == 2);
    if (!dhresult::check(done, error, [this] { next_state(&drain_ctl::do_target_pos_error); }))
        return;
    next_state(&drain_ctl::wait_new_position);
}

void drain_ctl::do_target_pos_error()
{
    hw_.log_message(LogMsg::Error, name(), fmt::format("Не удалось переключить в положение {}", target_pos_));
    target_pos_ = 0;
    next_state(&drain_ctl::wait_new_position);
}


void drain_ctl::update_state(bool drain, bool tank) noexcept
{
    if (drain_ == drain && tank_ == tank)
        return;
    
    drain_ = drain;
    tank_ = tank;
    
    position_ = drain_ ? 3 : (tank_ ? 1 : 2);
}

