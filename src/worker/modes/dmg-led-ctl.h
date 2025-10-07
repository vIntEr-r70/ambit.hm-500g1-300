#pragma once

#include <state-ctrl.h>
#include <drivers/bit-driver.h>

#include <optional>

namespace we {
    class hardware;
}

class dmg_led_ctl final
    : public state_ctrl<dmg_led_ctl>
{
public:

    dmg_led_ctl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void wait_state_changed() noexcept;

    void make_led_on_off() noexcept;

private:

    void update();
    
private:

    std::string sid_;
    we::hardware &hw_;
    
    bool fc_state_{ false };
    bool locker_state_{ false };
    bool emg_stop_state_{ false };
    
    std::optional<bool> new_led_state_;
    bool led_on_{ false };
    
    engine::bit_driver led_;
};
