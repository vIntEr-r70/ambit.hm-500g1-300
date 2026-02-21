#include "panel-rcu-switch.hpp"

panel_rcu_switch::panel_rcu_switch()
    : eng::sibus::node("panel-rcu-switch")
{
    node::add_input_port_unsafe("in", [this](eng::abc::pack args)
    {
        update_output(std::move(args));
    });

    panel_ = node::add_output_port("panel");
    rcu_ = node::add_output_port("rcu");
    rcu_led_ = node::add_output_port("rcu-led");

    mode_ = node::add_output_port("mode");
}

void panel_rcu_switch::update_output(eng::abc::pack args)
{
    if (!args)
    {
        node::set_port_value(panel_, { false });
        node::set_port_value(rcu_, { false });

        node::set_port_value(mode_, { "" });
        node::set_port_value(rcu_led_, { false });
    }
    else
    {
        bool rcu = eng::abc::get<bool>(args);

        node::set_port_value(rcu_, { rcu });
        node::set_port_value(panel_, { !rcu });

        node::set_port_value(mode_, { rcu ? "rcu" : "panel" });
        node::set_port_value(rcu_led_, { rcu });
    }
}
