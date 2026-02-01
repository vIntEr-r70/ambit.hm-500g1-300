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

    // // Успешный ответ на вызов с аргументами
    // node::set_wire_response_handler(o_ctl_, [this](bool success, eng::abc::pack args)
    // {
    //     if (success) return;
    //     eng::log::error("{}: {}", name(), eng::abc::get_sv(args));
    // });
}

void axis_panel_ctl::spin_command(double speed)
{
    eng::log::info("{}: spin_command: speed = {}", name(), speed);

    if (node::is_allow(ctl_))
        node::activate(ctl_, { "spin", speed });
}
