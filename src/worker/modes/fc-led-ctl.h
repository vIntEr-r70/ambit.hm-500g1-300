#pragma once

#include <state-ctrl.h>
#include <drivers/bit-driver.h>

#include <optional>

namespace we {
    class hardware;
}

class fc_led_ctl final
    : public state_ctrl<fc_led_ctl>
{
public:

    fc_led_ctl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void wait_state_changed() noexcept;

    void make_led_on_off() noexcept;

private:

    std::string sid_;
    we::hardware &hw_;
    
    std::optional<bool> new_led_state_;
    bool led_on_{ false };
    
    engine::bit_driver led_;
};

