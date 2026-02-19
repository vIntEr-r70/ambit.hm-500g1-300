#include "bki-ctl.hpp"

#include <eng/sibus/client.hpp>
#include <eng/json.hpp>
#include <eng/log.hpp>

bki_ctl::bki_ctl()
    : eng::sibus::node("bki-ctl")
{
    ictl_ = node::add_input_wire();

    node::set_activate_handler(ictl_, [this](eng::abc::pack args)
    {
        switch(eng::abc::get<std::uint8_t>(args))
        {
        case 0: // Игнорируем
            break;

        case 1: // Снимаем защиту
            eng::log::info("{}: Снять защиту", name());
            node::set_port_value(not_allow_, { true });
            node::set_port_value(reset_, { true });
            node::set_port_value(bki_, { 2 });
            wait_for_bki_activate_back_ = true;
            break;

        case 2: // Выставляем защиту
            eng::log::info("{}: Активировать защиту", name());
            node::set_port_value(not_allow_, { false });
            node::set_port_value(reset_, { false });
            wait_for_bki_activate_back_.reset();
            node::set_port_value(bki_, { 0 });
            break;
        }

        node::ready(ictl_);
    });

    reset_ = node::add_output_port("reset");
    bki_ = node::add_output_port("bki");

    node::add_input_port("bki", [this](eng::abc::pack args)
    {
        bool active = !eng::abc::get<bool>(args);
        eng::log::info("{}: active = {}", name(), active);

        if (wait_for_bki_activate_back_)
            return;

        eng::log::info("{}: PASS", name(), active);

        std::uint8_t key = active ? 1 : 0;
        node::set_port_value(bki_, { key });
    });

    not_allow_ = node::add_output_port("not-allow");

    eng::sibus::client::config_listener("bki", [this](std::string_view json)
    {
        // Сохраняем дефолтную конфигурацию
        if (json.empty())
            return;

        eng::json::value cfg(json);

        cfg_bki_allow_ = cfg.get<bool>();

        node::set_port_value(not_allow_, { !cfg_bki_allow_ });
        node::set_port_value(reset_, { !cfg_bki_allow_ });

        eng::log::info("{}: cfg_bki_allow = {}", name(), cfg_bki_allow_);
    });
}

void bki_ctl::register_on_bus_done()
{
    node::ready(ictl_);
}

