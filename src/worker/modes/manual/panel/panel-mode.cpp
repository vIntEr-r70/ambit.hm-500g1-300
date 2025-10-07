#include "panel-mode.h"

#include <hardware.h>

panel_mode::panel_mode(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("panel-mode")
    , hw_(hw)
    , cnc_(hw.get_unit_by_class("", "cnc"))
    , calibrate_ctrl_(hw, axis_cfg)
    , centering_ctrl_(hw, axis_cfg)
    , move_to_ctrl_(hw, axis_cfg)
    , panel_ctrl_(hw, axis_cfg)
{ 
    hw.SET.add("cnc", "axis-stop-all", [this](nlohmann::json const &) {
        need_stop_ = true;
    });
    
    hw.SET.add("cnc", "axis-init-zero", [this](nlohmann::json const &args) {
        zero_axis_ = args[0].get<char>();
    });
}

void panel_mode::on_activate() noexcept
{
    zero_axis_ = '\0';
    need_stop_ = false;

    panel_ctrl_.sync_state();
    move_to_ctrl_.sync_state();
    centering_ctrl_.sync_state();
    calibrate_ctrl_.sync_state();

    panel_ctrl_.activate();
    next_state(&panel_mode::in_panel_ctrl);
}

void panel_mode::on_deactivate() noexcept
{
    aem::log::trace("panel-mode::on_deactivate");

    centering_ctrl_.deactivate();
    calibrate_ctrl_.deactivate();
    panel_ctrl_.deactivate();
    move_to_ctrl_.deactivate();

    next_state(&panel_mode::wait_deactivate);
}

void panel_mode::wait_deactivate() noexcept
{
    if (!panel_ctrl_.is_done())
        return;
    if (!centering_ctrl_.is_done())
        return;
    if (!calibrate_ctrl_.is_done())
        return;
    if (!move_to_ctrl_.is_done())
        return;
    
    next_state(nullptr);
}

void panel_mode::in_panel_ctrl() noexcept
{
    if (zero_axis_ != '\0')
    {
        proc_zero_axis_ = zero_axis_;
        zero_axis_ = '\0';

        next_state(&panel_mode::set_as_zero);
        return;
    }

    if (move_to_ctrl_.need_move())
    {
        action_state_ = &panel_mode::init_move_to_ctrl;
        panel_ctrl_.deactivate();
        next_state(&panel_mode::wait_panel_ctrl_done);
        return;
    }

    if (calibrate_ctrl_.has_axis())
    {
        action_state_ = &panel_mode::init_calbrate_ctrl;
        panel_ctrl_.deactivate();
        next_state(&panel_mode::wait_panel_ctrl_done);
        return;
    }

    if (centering_ctrl_.ctype() != centering_type::not_active)
    {
        action_state_ = &panel_mode::init_centering_ctrl;
        panel_ctrl_.deactivate();
        next_state(&panel_mode::wait_panel_ctrl_done);
        return;
    }
}

void panel_mode::wait_panel_ctrl_done() noexcept
{
    if (!panel_ctrl_.is_done())
        return;
    next_state(action_state_);
}

void panel_mode::init_calbrate_ctrl() noexcept
{
    calibrate_ctrl_.activate();
    next_state(&panel_mode::in_calibrate_ctrl);
}

void panel_mode::init_centering_ctrl() noexcept
{
    centering_ctrl_.activate();
    next_state(&panel_mode::in_centering_ctrl);
}

void panel_mode::init_move_to_ctrl() noexcept
{
    need_stop_ = false;

    move_to_ctrl_.activate();
    next_state(&panel_mode::in_move_to_ctrl);
}

void panel_mode::in_move_to_ctrl() noexcept
{
    if (!move_to_ctrl_.is_done() && !need_stop_)
        return;

    need_stop_ = false;
    move_to_ctrl_.deactivate();

    panel_ctrl_.activate();
    next_state(&panel_mode::in_panel_ctrl);
}

void panel_mode::in_centering_ctrl() noexcept
{
    if (centering_ctrl_.ctype() == centering_type::not_active && centering_ctrl_.is_active())
        centering_ctrl_.deactivate();

    if (!centering_ctrl_.is_done())
        return;

    panel_ctrl_.activate();
    next_state(&panel_mode::in_panel_ctrl);
}

void panel_mode::in_calibrate_ctrl() noexcept
{
    // Если пользователь отменил процесс калибровки
    if (!calibrate_ctrl_.has_axis() && calibrate_ctrl_.is_active())
        calibrate_ctrl_.deactivate();

    if (!calibrate_ctrl_.is_done())
        return;

    panel_ctrl_.activate();
    next_state(&panel_mode::in_panel_ctrl);
}

void panel_mode::set_as_zero() noexcept
{
    auto [ done, error ] = cnc_.set_as_zero(proc_zero_axis_);
    if (!dhresult::check(done, error, [this] { next_state(&panel_mode::in_panel_ctrl); }))
        return;
    proc_zero_axis_ = '\0';
    next_state(&panel_mode::in_panel_ctrl);
}
