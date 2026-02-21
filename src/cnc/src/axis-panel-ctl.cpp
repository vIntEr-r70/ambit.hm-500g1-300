#include "axis-panel-ctl.hpp"

#include <eng/log.hpp>

axis_panel_ctl::axis_panel_ctl(char axis)
    : eng::sibus::node(std::format("axis-panel-ctl-{}", axis))
{
    node::add_input_port("ccw", [this](eng::abc::pack value)
    {
        bool spin = eng::abc::get<bool>(value);
        spin_command(spin ? -speed_ : 0.0);
    });

    node::add_input_port("cw", [this](eng::abc::pack value)
    {
        bool spin = eng::abc::get<bool>(value);
        spin_command(spin ? speed_ : 0.0);
    });

    node::add_input_port_unsafe("set-speed", [this](eng::abc::pack args)
    {
        speed_ = args ? eng::abc::get<double>(args) : 0.0;
    });

    ctl_ = node::add_output_wire();
}

void axis_panel_ctl::spin_command(double speed)
{
    node::activate(ctl_, { "spin", speed });
}
