#include "axis-panel-ctl.hpp"

#include <eng/log.hpp>

axis_panel_ctl::axis_panel_ctl(char axis)
    : eng::sibus::node(std::format("axis-panel-ctl-{}", axis))
{
    add_input_port("ccw", [this](eng::abc::pack value)
    {
        bool spin = eng::abc::get<bool>(value);
        spin_command(spin ? -speed_ : 0.0);
    });

    add_input_port("cw", [this](eng::abc::pack value)
    {
        bool spin = eng::abc::get<bool>(value);
        spin_command(spin ? speed_ : 0.0);
    });

    add_input_port("set-speed", [this](eng::abc::pack value)
    {
        speed_ = eng::abc::get<double>(value);
    });

    ctl_ = node::add_output_wire();
}

// TODO: Необходимо реализовать свое состояние в любом случае
void axis_panel_ctl::spin_command(double speed)
{
    eng::log::info("{}: spin_command: speed = {}", name(), speed);

    if (speed == 0.0 && node::is_active(ctl_))
        node::deactivate(ctl_);
    else if (node::is_ready(ctl_))
        node::activate(ctl_, { "spin", speed });
}
