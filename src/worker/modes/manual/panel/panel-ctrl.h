#pragma once

#include "../base-ctrl.h"

#include <state-ctrl.h>
#include <drivers/bit-driver.h>

class panel_ctrl
    : public state_ctrl<panel_ctrl>
    , public base_ctrl
{
public:

    panel_ctrl(we::hardware &, we::axis_cfg const &) noexcept;

public:

    void sync_state() noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void independent_move(char, int, bool);
    
private:

    void init_moving() noexcept;

    void pair_init_moving() noexcept;

    void turn_off_led() noexcept;
        
    void ctrl_allow() noexcept;

    void switch_to_independent_mode() noexcept;

    void pair_switch_to_independent_mode() noexcept;

    void stop_independent_move() noexcept;

    void pair_stop_independent_move() noexcept;

    void independent_move() noexcept;

    void pair_independent_move() noexcept;

    void stop_moving() noexcept;

    void independent_error() noexcept;

    float get_speed(we::axis_desc const &, std::size_t) const noexcept override final;
    
private:

    cnc_driver cnc_;
    engine::bit_driver rcu_led_;

    // Наше внутреннее состояние независимого движения
    std::map<char, int> in_proc_;

    // Текущее обрабатываемое независимое движение
    std::pair<char, int> axis_;

    // Текущая скорость
    float move_speed_;

    // Состояние парных осей
    std::unordered_map<char, float> pair_axis_;

    // Состояние, задаваемое извне
    std::map<char, int> tsp_state_;
    std::unordered_map<char, std::array<bool, 2>> bit_state_;
};
