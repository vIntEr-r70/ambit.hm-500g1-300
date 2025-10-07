#pragma once

#include <state-ctrl.h>
#include <drivers/bit-driver.h>
#include <optional>

namespace we 
{
    class hardware;
}

class drainage_ctl final
    : public state_ctrl<drainage_ctl>
{
public:

    drainage_ctl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void wait_state_changed();

    void turn_on_off_pump();
    
    void turn_on_off_pump_error();
    
private:

    std::string sid_;
    we::hardware &hw_;

    std::optional<bool> pump_;
    bool pump_on_off_;
    
    engine::bit_driver H7_ctl_;
};

