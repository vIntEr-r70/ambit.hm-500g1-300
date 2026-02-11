#include "barrel-ctl.hpp"

#include <eng/sibus/client.hpp>
#include <eng/json.hpp>
#include <eng/log.hpp>

// Данный модуль поддерживает температуру в заданном конфигурацией диапазоне

barrel_ctl::barrel_ctl(std::string_view name)
    : eng::sibus::node(name)
    // , add_liquid_ctl_(hw.get_unit_by_class(sid, "add-liquid"), "add-liquid")
{
    // Текущее значение температуры
    node::add_input_port("DT", [this](eng::abc::pack args)
    {
        current_temperature_ = eng::abc::get<double>(args);
        if (in_range_) (this->*in_range_)();
    });

    eng::sibus::client::config_listener(name, [this](std::string_view json)
    {
        eng::json::object cfg(json);

        cfg_.use_heater = cfg.get<bool>("use-heater", false);
        cfg_.use_cooler = cfg.get<bool>("use-cooler", false);

        cfg_.pump_full_time = cfg.get<bool>("pump-full-time", false);

        cfg_.dt_min = cfg.get<double>("dt-min", 20.0);
        cfg_.dt_max = cfg.get<double>("dt-max", 80.0);

        if (in_range_) (this->*in_range_)();
    });

    pump_out_ = node::add_output_port("H");
    heater_out_ = node::add_output_port("WH");
    valve_out_ = node::add_output_port("V");

    // // Читаем текущее общее долитое количество жидкости
    // hw.LSET.add(sid, "liquid", [this](nlohmann::json const &value) {
    //     hw_.notify({ name(), "liquid" }, value);
    // });
    // // Читаем текущее оставшееся долиться количество жидкости
    // hw.LSET.add(sid, "add-liquid", [this](nlohmann::json const &value) {
    //     hw_.notify({ name(), "add-liquid" }, value);
    // });
    //
    // hw.START.add(sid, [this]
    // {
    //     activate_ = true;
    // });
    //
    // hw.STOP.add(sid, [this]
    // {
    //     activate_ = false;
    // });
    //
    // // Команда долить жидкости
    // hw.SET.add(sid, "add-liquid", [this](nlohmann::json const &json)
    // {
    // 	liquid_ = static_cast<std::uint16_t>(json.get<double>());
    //     hw_.log_message(LogMsg::Info, name(), fmt::format("Доливаем {} литров", liquid_.value()));
    // });
}

void barrel_ctl::update_liquid_state()
{
    // if (надо долить жидкости)
    // {
    //     команда долить жидкости
    //     return;
    // }
}

// Мы в диапазоне заданной температуры
void barrel_ctl::in_temperature_range()
{
    if (need_turn_on_heater())
    {
        eng::log::info("{}: Включаем нагреватель", name());

        node::set_port_value(pump_out_, { true });
        node::set_port_value(heater_out_, { true });

        in_range_ = &barrel_ctl::in_heater_range;
    }
    else if (need_turn_on_cooler())
    {
        eng::log::info("{}: Включаем охладитель", name());

        node::set_port_value(pump_out_, { true });
        node::set_port_value(valve_out_, { true });

        in_range_ = &barrel_ctl::in_cooler_range;
    }
}

void barrel_ctl::in_heater_range()
{
    if (need_stay_in_heater_range())
        return;

    eng::log::info("{}: Выключаем нагреватель", name());

    node::set_port_value(pump_out_, { cfg_.pump_full_time });
    node::set_port_value(heater_out_, { false });

    in_range_ = &barrel_ctl::in_temperature_range;
}

void barrel_ctl::in_cooler_range()
{
    if (need_stay_in_cooler_range())
        return;

    eng::log::info("{}: Выключаем охладитель", name());

    node::set_port_value(pump_out_, { cfg_.pump_full_time });
    node::set_port_value(valve_out_, { false });

    in_range_ = &barrel_ctl::in_temperature_range;
}

// void barrel_ctl::add_liquid()
// {
//     auto [ done, error ] = add_liquid_ctl_.set(add_liquid_value_);
//     if (!dhresult::check(done, error, [this] { next_state(&barrel_ctl::add_liquid_error); }))
//         return;
//     next_state(back_state_);
// }



// bool barrel_ctl::need_stop()
// {
//     if (activate_)
//     {
//         active_ = activate_.value();
//         activate_.reset();
//     }
//     return !active_;
// }

// bool barrel_ctl::need_liquid()
// {
//     if (!liquid_)
//         return false;
//
//     add_liquid_value_ = liquid_.value();
//     liquid_.reset();
//
//     return true;
// }

