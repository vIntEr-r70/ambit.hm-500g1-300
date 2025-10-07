#include "dmg-led-ctl.h"

#include <hardware.h>
#include <aem/log.h>

dmg_led_ctl::dmg_led_ctl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , hw_(hw)
    , led_(hw.get_unit_by_class(sid, "dmg-led"), "dmg-led")
{ 
    // Текущий статус ПЧ
    hw.LSET.add(sid, "STAT", [this](nlohmann::json const &json) 
    {
        fc_state_ = json.get<std::size_t>() == 2;
        update();
    });

    // Состояние блокировок критическое 
    hw.listen_event(sid, "state", [this](nlohmann::json const &json) 
    {
        locker_state_ = (json.get<bool>() == true);
        update();
    });
    
    hw.LSET.add(sid, "emg-stop", [this](nlohmann::json const &json) 
    {
        emg_stop_state_ = (json.get<bool>() == true);
        update();
    });
}

void dmg_led_ctl::update()
{
    new_led_state_ = fc_state_ || locker_state_ || emg_stop_state_;
}

void dmg_led_ctl::on_activate() noexcept
{
    led_on_ = false;
    next_state(&dmg_led_ctl::make_led_on_off);
}

void dmg_led_ctl::on_deactivate() noexcept
{ }

void dmg_led_ctl::wait_state_changed() noexcept
{
    if (!new_led_state_) 
        return;
    
    led_on_ = new_led_state_.value();
    new_led_state_.reset();
    
    next_state(&dmg_led_ctl::make_led_on_off);
}

void dmg_led_ctl::make_led_on_off() noexcept
{
    auto [ done, error ] = led_.set(led_on_);
    if (!dhresult::check(done, error, [this] { next_state(&dmg_led_ctl::wait_state_changed); }))
        return;
    next_state(&dmg_led_ctl::wait_state_changed);
}
