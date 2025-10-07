#pragma once

#include <state-ctrl.h>
#include <drivers/bit-driver.h>

#include "../base-ctrl.h"

#include <optional>

class rcu_ctrl
    : public state_ctrl<rcu_ctrl>
    , public base_ctrl
{
public:

    rcu_ctrl(we::hardware &, we::axis_cfg const &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void turn_on_led() noexcept;

    void ctrl_allow() noexcept;

    void prepare_target_move() noexcept;

    void switch_to_target_mode() noexcept;

    void stop_target_move() noexcept;

    void reset_queue() noexcept;

    void wait_target_move_stop() noexcept;

    void relative_target_move() noexcept;

    void ctrl_error() noexcept;

private:

    void set_axis_tsp() noexcept;

    void update_internal_axis_cfg() noexcept override final;

    bool is_new_axis_cfg() const noexcept override final;

    float get_real_shift_from_cfg(char, int) const noexcept;
    
    float get_speed(we::axis_desc const &, std::size_t) const noexcept override final;

private:

    // Последнее пришедшее от пульта значение
    std::optional<int> pos_;
    
    // Изменение положения диска на переносном пульте
    int spin_{ 0 };

    // Реальная ось исходя из конфигурации
    char axis_;

    // Текущее обрабатываемое смещение
    int shift_;

    // Последняя ось, которой отдавали команду
    char last_axis_;
    // Последнее значение смещения
    int last_shift_;

    // Положение тумблера выбора оси на переносном пульте
    char axis_tsp_{ 0 };
    char real_axis_tsp_{ 0 };

    cnc_driver cnc_;
    engine::bit_driver rcu_led_;

    std::unordered_map<char, bool> akeys_;
};

