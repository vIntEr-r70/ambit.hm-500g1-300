#include "panel-switch-2-ctl.hpp"

#include <eng/log.hpp>

panel_switch_2_ctl::panel_switch_2_ctl(std::string_view name)
    : eng::sibus::node(name)
{
    // Входной дискрет
    node::add_input_port("in", [this](eng::abc::pack args)
    {
        bool state = eng::abc::get<bool>(args);
        deactivate(state ? 0 : 1);
        activate(state ? 1 : 0);
    });

    // Управляющий провод
    ctl_[0] = node::add_output_wire();
    ctl_[1] = node::add_output_wire();
}

void panel_switch_2_ctl::activate(std::size_t idx)
{
    if (!node::is_allow(ctl_[idx]))
    {
        eng::log::error("{}: Состояние {} не доступно для активации", name(), idx);
        return;
    }
    eng::log::info("{}: Активировано {}", name(), idx);
    node::activate(ctl_[idx], { });
}

void panel_switch_2_ctl::deactivate(std::size_t idx)
{
    if (!node::is_allow(ctl_[idx]))
    {
        eng::log::error("{}: Состояние {} не доступно для деактивации", name(), idx);
        return;
    }
    eng::log::info("{}: ДеАктивировано {}", name(), idx);
    node::activate(ctl_[idx], { });
}

