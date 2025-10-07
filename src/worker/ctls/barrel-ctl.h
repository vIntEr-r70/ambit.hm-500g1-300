#pragma once

#include <state-ctrl.h>
#include <drivers/reg-driver.h>
#include <drivers/bit-driver.h>
#include <optional>

namespace we 
{
    class hardware;
}

class barrel_ctl final
    : public state_ctrl<barrel_ctl>
{
public:

    barrel_ctl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void wait_add_liquid();

    void in_range();

    void in_heater_range();
    
    void in_cooler_range();
    
    void add_liquid();
    void add_liquid_error();

    void turn_on_pump();
    void turn_on_pump_error();
    
    void turn_on_heater();
    void turn_on_heater_error();
        
    void turn_on_cooler();
    void turn_on_cooler_error();

    void turn_off_pump();
    void turn_off_pump_error();
    
    void turn_off_heater();
    void turn_off_heater_error();
    
    void turn_off_cooler();
    void turn_off_cooler_error();
    
private:
    
    bool need_stop();
    
    bool need_liquid();
    
    bool need_turn_on_heater() const;
    
    bool need_turn_on_cooler() const;

    float target_temp_value() const;
        
private:

    std::string sid_;
    we::hardware &hw_;

    std::optional<bool> activate_;
    bool active_;
    
    std::optional<std::uint16_t> liquid_;
    std::uint16_t add_liquid_value_;
    
    engine::bit_driver pump_ctl_;
    engine::bit_driver cooler_ctl_;
    engine::bit_driver heater_ctl_;

    void(barrel_ctl::*required_impact_)();
    void(barrel_ctl::*required_state_)();
    void(barrel_ctl::*back_state_)();
    
    engine::reg_driver add_liquid_ctl_;

    float temp_;
    float temp_min_;
    float temp_max_;

    bool use_heater_;
    bool use_cooler_;
    bool full_time_pump_;
};

