#pragma once

#include <state-ctrl.h>
#include <drivers/bit-driver.h>
#include <optional>

namespace we {
    class hardware;
}

class spump_ctl 
    : public state_ctrl<spump_ctl>
{
public:

    spump_ctl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void turn_all_off() noexcept;

    void turn_on_env_pump() noexcept;
    
    void turned_on() noexcept;
    
    void turn_on_wtr_pump() noexcept;
    
    void turned_off() noexcept;
    
    void turn_on_led() noexcept;
    
private:
    
    void set_error(std::string_view) noexcept;
    
protected:

    std::string sid_;
    we::hardware &hw_;

private:
    
    std::optional<bool> cmd_;
    bool tank_{ false };
    
    engine::bit_driver env_;
    engine::bit_driver wtr_;
    engine::bit_driver led_;
    
    std::vector<engine::bit_driver*> init_tasks_;
};
