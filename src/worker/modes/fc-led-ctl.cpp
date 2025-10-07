#include "fc-led-ctl.h"

#include <hardware.h>
#include <aem/log.h>

fc_led_ctl::fc_led_ctl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , hw_(hw)
    , led_(hw.get_unit_by_class(sid, "fc-led"), "fc-led")
{ 
    // Текущий статус ПЧ
    hw.LSET.add(sid, "STAT", [this](nlohmann::json const &json) {
        new_led_state_ = json.get<std::size_t>() == 1;
    });
    
    // Наличие / отсутствие связи с ПЧ
    hw.listen_event(sid, "offline", [this](nlohmann::json const &value) 
    {
        if (!value.get<bool>())
            return;
            
        if (led_on_ || new_led_state_)
            new_led_state_ = false;
    });
}

void fc_led_ctl::on_activate() noexcept
{
    led_on_ = false;
    next_state(&fc_led_ctl::make_led_on_off);
}

void fc_led_ctl::on_deactivate() noexcept
{ }

void fc_led_ctl::wait_state_changed() noexcept
{
    if (!new_led_state_) 
        return;
    
    led_on_ = new_led_state_.value();
    new_led_state_.reset();
    
    next_state(&fc_led_ctl::make_led_on_off);
}

void fc_led_ctl::make_led_on_off() noexcept
{
    auto [ done, error ] = led_.set(led_on_);
    if (!dhresult::check(done, error, [this] { next_state(&fc_led_ctl::wait_state_changed); }))
        return;
    next_state(&fc_led_ctl::wait_state_changed);
}


