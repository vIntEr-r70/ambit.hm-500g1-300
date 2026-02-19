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

void axis_panel_ctl::spin_command(double speed)
{
    if (node::is_blocked(ctl_))
        return;
    node::activate(ctl_, { "spin", speed });
}
