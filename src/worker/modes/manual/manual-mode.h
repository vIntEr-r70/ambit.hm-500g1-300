#pragma once

#include <state-ctrl.h>
#include "common/Defs.h"

#include <aem/types.h>

#include "panel/panel-mode.h"
#include "rcu/rcu-ctrl.h"

class manual_mode
    : public state_ctrl<manual_mode>
{
public:

    manual_mode(we::hardware &, we::axis_cfg const &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void update_manual_mode() noexcept;

    void in_rcu_mode() noexcept;

    void wait_rcu_ctrl_done() noexcept;

    void in_panel_mode() noexcept;

    void wait_panel_mode_done() noexcept;

    void wait_deactivate() noexcept;

private:

    void update_sys_ctrl(aem::uint8, bool = false) noexcept;

private:

    we::hardware &hw_;

    aem::uint8 sys_ctrl_{ Core::CtlMode::No };

public:

    // Флаг разрешения управления с переносного пульта
    bool rcu_enabled_{ false };

    // Управление двигателями с панели
    panel_mode panel_mode_;

    // Управление двигателями с пульта
    rcu_ctrl rcu_ctrl_;
};

