#include "panel-button-ctl.hpp"

#include <eng/log.hpp>

panel_button_ctl::panel_button_ctl(std::string_view name)
    : eng::sibus::node(name)
{
    // Вход команды включения
    node::add_input_port("0", [this](eng::abc::pack args)
    {
        if (eng::abc::get<bool>(args) == true)
            turn_on_action();
    });

    // Вход команды выключения
    node::add_input_port("1", [this](eng::abc::pack args)
    {
        if (eng::abc::get<bool>(args) == false)
            turn_off_action();
    });

    // Управляющий провод
    out_ = node::add_output_port("out");
}

void panel_button_ctl::turn_on_action()
{
    node::set_port_value(out_, { true });
}

void panel_button_ctl::turn_off_action()
{
    node::set_port_value(out_, { false });
}


