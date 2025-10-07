#include "unit-on-off-ctl.h"

#include <hardware.h>
#include <aem/log.h>

unit_on_off_ctl::unit_on_off_ctl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , sid_(sid)
    , hw_(hw)
    , power_(hw.get_unit_by_class(sid, "power"), sid)
    , led_(hw.get_unit_by_class(sid, "led"), sid)
{ 
    hw.START.add(sid_, [this] 
    { 
        if (blocked_ ) return;
        hw_.log_message(LogMsg::Command, sid_, "Включение");
        cmd_ = true; 
    });

    hw.STOP.add(sid_, [this] 
    { 
        if (blocked_ ) return;
        hw_.log_message(LogMsg::Command, sid_, "Выключение");
        cmd_ = false; 
    });

    hw.LSET.add(sid_, "start", [this](nlohmann::json const &value)
    { 
        if (!value.get<bool>() || blocked_ ) return;
        hw_.log_message(LogMsg::Command, sid_, "Включение");
        cmd_ = true; 
    });
    
    hw.LSET.add(sid_, "stop", [this](nlohmann::json const &value) 
    { 
        if (!value.get<bool>() || blocked_ ) return;
        hw_.log_message(LogMsg::Command, sid_, "Выключение");
        cmd_ = false; 
    });
    
    hw.listen_event(sid_, "block-manual", [this](nlohmann::json const &value) 
    {
        blocked_ = value.get<bool>(); 
        if (blocked_) cmd_ = false;
    });
}

void unit_on_off_ctl::on_activate() noexcept
{
    power_.reset();
    led_.reset();

    ustate_ = false;
    hw_.notify({ sid_, "V" }, ustate_);

    next_state(&unit_on_off_ctl::update_power_state);
}

void unit_on_off_ctl::on_deactivate() noexcept
{ }

void unit_on_off_ctl::wait_state_changed() noexcept
{
    if (!is_active())
    {
        next_state(nullptr);
        return;
    }
    
    if (is_locked())
    {
        if (ustate_)
            cmd_ = false;
        else
            cmd_.reset();
    }

    if (!cmd_.has_value())
        return;

    ustate_ = cmd_.value();
    cmd_.reset();
    
    hw_.notify({ sid_, "V" }, ustate_);

    next_state(&unit_on_off_ctl::update_power_state);
}

void unit_on_off_ctl::update_power_state() noexcept
{
    auto [ done, error ] = power_.set(ustate_);
    if (!dhresult::check(done, error, [this] { next_state(&unit_on_off_ctl::state_error); }))
        return;
    next_state(&unit_on_off_ctl::update_led_state);
}

void unit_on_off_ctl::update_led_state() noexcept
{
    auto [ done, error ] = led_.set(ustate_);
    if (!dhresult::check(done, error, [this] { next_state(&unit_on_off_ctl::wait_state_changed); }))
        return;
    next_state(&unit_on_off_ctl::wait_state_changed);
}

void unit_on_off_ctl::state_error() noexcept
{
    hw_.log_message(LogMsg::Error, sid_, "Не удалось выполнить команду");
    next_state(&unit_on_off_ctl::wait_state_changed);
}
