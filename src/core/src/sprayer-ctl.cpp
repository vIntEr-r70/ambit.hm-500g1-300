#include "sprayer-ctl.hpp"

sprayer_ctl::sprayer_ctl(std::string_view name)
    : eng::sibus::node(name)
{
    ictl_ = node::add_input_wire();

    node::set_activate_handler(ictl_, [this](eng::abc::pack)
    {
        activate();
    });
    node::set_deactivate_handler(ictl_, [this]
    {
        deactivate();
    });

    on_off_ = node::add_output_port("on-off");

    node::add_input_port_unsafe("state", [this](eng::abc::pack args)
    {
        // bool link = args.size() != 0;
        // bool powered = link ? eng::abc::get<bool>(args) : false;
        // update_real_state(link, powered);
    });
}

void sprayer_ctl::activate()
{
    // if (!linked_)
    // {
    //     node::reject(ictl_, "Отсутствует связь с драйвером");
    //     return;
    // }

    powered_ = true;
    node::set_port_value(on_off_, { true });
}

void sprayer_ctl::deactivate()
{
    powered_ = false;
    node::set_port_value(on_off_, { false });

    // if (!linked_)
    //     node::terminate(ictl_, "Отсутствует связь с драйвером");
}

void sprayer_ctl::update_real_state(bool link, bool powered)
{
    // if (linked_)
    // {
    //     if (!link)
    //     {
    //         linked_ = false;
    //         node::terminate(ictl_, "Отсутствует связь с драйвером");
    //
    //         if (powered_)
    //         {
    //             powered_ = false;
    //             node::set_port_value(on_off_, { false });
    //         }
    //
    //         return;
    //     }
    // }
    // else if (link)
    // {
    //     linked_ = true;
    //     node::ready(ictl_);
    // }
    //
    // if (powered != powered_ && !powered_)
    //     node::set_port_value(on_off_, { false });
}

void sprayer_ctl::register_on_bus_done()
{
    node::ready(ictl_);
}

