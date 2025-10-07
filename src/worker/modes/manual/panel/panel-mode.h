#pragma once

#include "move-to-ctrl.h"
#include "calibrate-ctrl.h"
#include "centering-ctrl.h"
#include "panel-ctrl.h"

class panel_mode final
    : public state_ctrl<panel_mode>
{
public:

    panel_mode(we::hardware &, we::axis_cfg const &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void wait_deactivate() noexcept;

    void in_panel_ctrl() noexcept;

    void wait_panel_ctrl_done() noexcept;

    void init_move_to_ctrl() noexcept;

    void in_move_to_ctrl() noexcept;

    void init_centering_ctrl() noexcept;

    void in_centering_ctrl() noexcept;
    
    void init_calbrate_ctrl() noexcept;

    void in_calibrate_ctrl() noexcept;

    void set_as_zero() noexcept;

private:

    handler_t action_state_{ nullptr };

    we::hardware &hw_;
    cnc_driver cnc_;

    char proc_zero_axis_;

public:

    // Режим калибровки
    calibrate_ctrl calibrate_ctrl_;

    // Режим поиска центра относительно детали
    centering_ctrl centering_ctrl_;

    // Движение в координату
    move_to_ctrl move_to_ctrl_;

    // Управление с панели
    panel_ctrl panel_ctrl_;

    char zero_axis_;
    bool need_stop_;
};
