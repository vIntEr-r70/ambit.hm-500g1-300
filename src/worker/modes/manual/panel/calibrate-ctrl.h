#pragma once

#include <state-ctrl.h>
#include <drivers/cnc-driver.h>

namespace we 
{
    class hardware;
    class axis_cfg;
}

class calibrate_ctrl final
    : public state_ctrl<calibrate_ctrl>
{
public:

    calibrate_ctrl(we::hardware &, we::axis_cfg const &) noexcept;

public:

    void sync_state() noexcept;

    char has_axis() const noexcept { return axis_ != '\0'; }

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void switch_to_independent_mode() noexcept;

    void limit_search_move() noexcept;

    void wait_limit_search_done() noexcept;

    void switch_to_target_mode() noexcept;

    void move_out_from_limit() noexcept;

    void wait_move_out_done() noexcept;

    void move_away_from_limit() noexcept;

    void wait_move_away_done() noexcept;

    void calibrate_done() noexcept;

    void calibrate_error() noexcept;
        
    void terminate_independent_move() noexcept;

    void terminate_target_move() noexcept;

    void reset_queue() noexcept;

    void wait_terminate_target_move_stop() noexcept;

private:

    void update_calibrate_step(std::size_t, bool = false) noexcept;

    void calibrate_step_error() noexcept;

    float get_limit_search_speed(std::size_t) const noexcept;

private:

    we::hardware &hw_;
    we::axis_cfg const &axis_cfg_;
    cnc_driver cnc_;

    // Скорость, с которой будет осуществлен поиск концевика
    float limit_search_speed_;

    // Обработчик, который должен будет вызваться когда концевик найден
    void(calibrate_ctrl::*on_limit_found_action_)();

    char real_axis_{ '\0' };

    int step_{ 0 };

    char axis_{ '\0' };
};

