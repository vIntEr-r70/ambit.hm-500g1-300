#pragma once

#include <state-ctrl.h>
#include <drivers/bit-driver.h>
#include <optional>

namespace we {
    class hardware;
}

class unit_on_off_ctl
    : public state_ctrl<unit_on_off_ctl>
{
public:

    unit_on_off_ctl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void wait_state_changed() noexcept;

    void update_power_state() noexcept;

    void update_led_state() noexcept;

    void state_error() noexcept;

private:
    
    virtual bool is_locked() const noexcept { return false; }
    
protected:

    std::string sid_;
    we::hardware &hw_;

private:
    
    std::optional<bool> cmd_;
    bool ustate_;

    engine::bit_driver power_;
    engine::bit_driver led_;

    bool blocked_{ false };
};
