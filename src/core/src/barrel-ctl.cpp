#include "barrel-ctl.hpp"

#include <eng/sibus/client.hpp>
#include <eng/json.hpp>
#include <eng/log.hpp>

// Данный модуль поддерживает температуру в заданном конфигурацией диапазоне
// И занимается доливкой жидкости в вверенную ему емкость

barrel_ctl::barrel_ctl(std::string_view key)
    : eng::sibus::node(std::format("barrel-ctl-{}", key))
    , in_range_(nullptr)
{
    // Текущее значение температуры
    node::add_input_port("DT", [this](eng::abc::pack args)
    {
        current_temperature_ = eng::abc::get<double>(args);
        if (in_range_) (this->*in_range_)();
    });

    node::add_input_port("on-off", [this](eng::abc::pack args)
    {
        if (eng::abc::get<bool>(args))
        {
            if (!in_range_)
                in_range_ = &barrel_ctl::in_temperature_range;

            if (current_temperature_)
                (this->*in_range_)();

            return;
        }

        deactivate();
    });

    eng::sibus::client::config_listener(std::format("barrel-ctl/{}", key), [this](std::string_view json)
    {
        eng::json::object cfg(json);

        cfg_.use_heater = cfg.get<bool>("use-heater", false);
        cfg_.use_cooler = cfg.get<bool>("use-cooler", false);

        cfg_.pump_full_time = cfg.get<bool>("pump-full-time", false);

        cfg_.dt_min = cfg.get<double>("dt-min", 20.0);
        cfg_.dt_max = cfg.get<double>("dt-max", 80.0);

        if (in_range_) (this->*in_range_)();
    });

    pump_ = node::add_output_port("pump");
    heater_ = node::add_output_port("heater");
    cooler_ = node::add_output_port("cooler");
}

void barrel_ctl::update_liquid_state()
{
}

void barrel_ctl::deactivate()
{
    node::set_port_value(pump_, { false });
    node::set_port_value(heater_, { false });
    node::set_port_value(cooler_, { false });

    in_range_ = nullptr;
}

// Мы в диапазоне заданной температуры
void barrel_ctl::in_temperature_range()
{
    if (need_turn_on_heater())
    {
        eng::log::info("{}: Включаем нагреватель", name());

        node::set_port_value(pump_, { true });
        node::set_port_value(heater_, { true });

        in_range_ = &barrel_ctl::in_heater_range;
    }
    else if (need_turn_on_cooler())
    {
        eng::log::info("{}: Включаем охладитель", name());

        node::set_port_value(pump_, { true });
        node::set_port_value(heater_, { true });

        in_range_ = &barrel_ctl::in_cooler_range;
    }
}

void barrel_ctl::in_heater_range()
{
    if (need_stay_in_heater_range())
        return;

    eng::log::info("{}: Выключаем нагреватель", name());

    node::set_port_value(pump_, { cfg_.pump_full_time });
    node::set_port_value(heater_, { false });

    in_range_ = &barrel_ctl::in_temperature_range;
}

void barrel_ctl::in_cooler_range()
{
    if (need_stay_in_cooler_range())
        return;

    eng::log::info("{}: Выключаем охладитель", name());

    node::set_port_value(pump_, { cfg_.pump_full_time });
    node::set_port_value(cooler_, { false });

    in_range_ = &barrel_ctl::in_temperature_range;
}

