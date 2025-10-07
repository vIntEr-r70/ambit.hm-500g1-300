#pragma once

#include "../base-ctrl.h"

#include <state-ctrl.h>
#include <drivers/cnc-driver.h>

class move_to_ctrl final 
    : public state_ctrl<move_to_ctrl>
    , public base_ctrl
{
public:

    move_to_ctrl(we::hardware &, we::axis_cfg const &) noexcept;

public:

    void sync_state() noexcept;

    void set_move_to(char, float) noexcept;

    bool need_move() noexcept { return move_to_.first != '\0'; }

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void ctrl_allow() noexcept;

    void switch_to_target_mode() noexcept;

    void target_move() noexcept;

    void stop_target_move() noexcept;

    void wait_target_move_stop() noexcept;

    void target_error() noexcept;

    void target_stop_error() noexcept;

    void reset_queue() noexcept;

    float get_speed(we::axis_desc const &, std::size_t) const noexcept override final;
    
private:

    cnc_driver cnc_;

    // Команды на линейное движение
    std::pair<char, float> move_to_; 

    // Текущее движение в позицию
    std::pair<char, float> real_move_to_;

    // Текущая скорость
    float real_speed_;
};

