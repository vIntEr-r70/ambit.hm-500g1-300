#include "manual-auto-switch.hpp"

manual_auto_switch::manual_auto_switch()
    : eng::sibus::node("manual-auto-switch")
{
    node::add_input_port_unsafe("in", [this](eng::abc::pack args)
    {
        update_output(std::move(args));
    });

    manual_ = node::add_output_port("manual");
    auto_ = node::add_output_port("auto");

    mode_ = node::add_output_port("mode");
}

void manual_auto_switch::update_output(eng::abc::pack args)
{
    if (!args)
    {
        node::set_port_value(manual_, { false });
        node::set_port_value(auto_, { false });

        node::set_port_value(mode_, { });
    }
    else
    {
        bool mauto = eng::abc::get<bool>(args);

        node::set_port_value(auto_, { mauto });
        node::set_port_value(manual_, { !mauto });

        node::set_port_value(mode_, { mauto ? "auto" : "manual" });
    }
}
