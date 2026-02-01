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
    ctl_ = node::add_output_wire();
}

void panel_button_ctl::turn_on_action()
{
    if (!node::is_allow(ctl_))
    {
        eng::log::error("{}: Включение с кнопки не доступно", name());
        return;
    }

    eng::log::info("{}: Включение", name());
    node::activate(ctl_, { });
}

void panel_button_ctl::turn_off_action()
{
    if (!node::is_allow(ctl_))
    {
        eng::log::error("{}: Выключение с кнопки не доступно", name());
        return;
    }

    eng::log::info("{}: Выключение", name());
    node::deactivate(ctl_);
}


