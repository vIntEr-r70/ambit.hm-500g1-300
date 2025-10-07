#include "manual-mode.h"

#include <hardware.h> 

manual_mode::manual_mode(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("manual-mode")
    , hw_(hw)
    , panel_mode_(hw, axis_cfg)
    , rcu_ctrl_(hw, axis_cfg)
{ 
    hw.LSET.add(name(), "mode-rcu", [this](nlohmann::json const &value) {
        rcu_enabled_ = value.get<bool>(); 
    });
}

void manual_mode::update_sys_ctrl(aem::uint8 sys_ctrl, bool force) noexcept
{
    if (sys_ctrl_ == sys_ctrl && !force)
        return;
    sys_ctrl_ = sys_ctrl;
    hw_.notify({ "sys", "ctrl" }, sys_ctrl_);
}

void manual_mode::on_activate() noexcept
{
    aem::log::trace("manual_mode::on_activate");

    update_sys_ctrl(Core::CtlMode::No, true);
    next_state(&manual_mode::update_manual_mode);
}

void manual_mode::on_deactivate() noexcept
{
    update_sys_ctrl(Core::CtlMode::No);

    rcu_ctrl_.deactivate();
    panel_mode_.deactivate();

    next_state(&manual_mode::wait_deactivate);
}

void manual_mode::wait_deactivate() noexcept
{
    if (!panel_mode_.is_done() || !rcu_ctrl_.is_done())
        return;
    next_state(nullptr);
}

void manual_mode::update_manual_mode() noexcept
{
    aem::log::trace("manual_mode::update_manual_mode");

    if (rcu_enabled_)
    {
        update_sys_ctrl(Core::CtlMode::Rcu);

        rcu_ctrl_.activate();
        next_state(&manual_mode::in_rcu_mode);
    }
    else
    {
        update_sys_ctrl(Core::CtlMode::Panel);

        panel_mode_.activate();
        next_state(&manual_mode::in_panel_mode);
    }
}

void manual_mode::in_rcu_mode() noexcept
{
    if (rcu_enabled_)
        return;
    rcu_ctrl_.deactivate();
    next_state(&manual_mode::wait_rcu_ctrl_done);
}

void manual_mode::wait_rcu_ctrl_done() noexcept
{
    if (!rcu_ctrl_.is_done())
        return;
    next_state(&manual_mode::update_manual_mode);
}

void manual_mode::in_panel_mode() noexcept
{
    if (!rcu_enabled_)
        return;
    panel_mode_.deactivate();
    next_state(&manual_mode::wait_panel_mode_done);
}

void manual_mode::wait_panel_mode_done() noexcept
{
    if (!panel_mode_.is_done())
        return;
    next_state(&manual_mode::update_manual_mode);
}
