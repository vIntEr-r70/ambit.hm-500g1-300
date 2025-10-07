#include "barrel-ctl.h"

#include <cmath>
#include <hardware.h>
#include <aem/log.h>

barrel_ctl::barrel_ctl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , hw_(hw)
    , add_liquid_ctl_(hw.get_unit_by_class(sid, "add-liquid"), "add-liquid")
    , pump_ctl_(hw.get_unit_by_class(sid, "pump"), "pump")
    , cooler_ctl_(hw.get_unit_by_class(sid, "valve"), "valve")
    , heater_ctl_(hw.get_unit_by_class(sid, "heater"), "heater")
{
    hw.LSET.add(sid, "dt", [this](nlohmann::json const &value)
    {
        temp_ = value.get<float>();
        hw_.notify({ name(), "dt" }, value);
    });
    // Читаем текущее общее долитое количество жидкости
    hw.LSET.add(sid, "liquid", [this](nlohmann::json const &value) {
        hw_.notify({ name(), "liquid" }, value);
    });
    // Читаем текущее оставшееся долиться количество жидкости
    hw.LSET.add(sid, "add-liquid", [this](nlohmann::json const &value) {
        hw_.notify({ name(), "add-liquid" }, value);
    });

    hw.START.add(sid, [this]
    {
        activate_ = true;
    });

    hw.STOP.add(sid, [this]
    {
        activate_ = false;
    });

    // Команда долить жидкости
    hw.SET.add(sid, "add-liquid", [this](nlohmann::json const &json)
    {
    	liquid_ = static_cast<std::uint16_t>(json.get<double>());
        hw_.log_message(LogMsg::Info, name(), fmt::format("Доливаем {} литров", liquid_.value()));
    });

    // Команда долить жидкости
    hw.SET.add(sid, "use-heater", [this](nlohmann::json const &json)
    {
        hw_.save(name(), "use-heater", json);
        use_heater_ = json.get<bool>();
        hw_.notify({ name(), "use-heater" }, use_heater_);
    });
    hw.SET.add(sid, "use-cooler", [this](nlohmann::json const &json)
    {
        hw_.save(name(), "use-cooler", json);
        use_cooler_ = json.get<bool>();
        hw_.notify({ name(), "use-cooler" }, use_cooler_);
    });
    hw.SET.add(sid, "full-time-pump", [this](nlohmann::json const &json)
    {
        hw_.save(name(), "full-time-pump", json);
        full_time_pump_ = json.get<bool>();
        hw_.notify({ name(), "full-time-pump" }, full_time_pump_);
    });
    hw.SET.add(sid, "dt-max", [this](nlohmann::json const &json)
    {
        hw_.save(name(), "dt-max", json);
        temp_max_ = json.get<float>();
        hw_.notify({ name(), "dt-max" }, temp_max_);
    });
    hw.SET.add(sid, "dt-min", [this](nlohmann::json const &json)
    {
        hw_.save(name(), "dt-min", json);
        temp_min_ = json.get<float>();
        hw_.notify({ name(), "dt-min" }, temp_min_);
    });

    use_heater_ = hw_.storage(name(), "use-heater", false);
    hw_.notify({ name(), "use-heater" }, use_heater_);

    use_cooler_ = hw_.storage(name(), "use-cooler", false);
    hw_.notify({ name(), "use-cooler" }, use_cooler_);

    full_time_pump_ = hw_.storage(name(), "full-time-pump", false);
    hw_.notify({ name(), "full-time-pump" }, full_time_pump_);

    temp_min_ = hw_.storage(name(), "dt-min", 20.0f);
    hw_.notify({ name(), "dt-min" }, temp_min_);

    temp_max_ = hw_.storage(name(), "dt-max", 80.0f);
    hw_.notify({ name(), "dt-max" }, temp_max_);
}

void barrel_ctl::on_activate() noexcept
{
    hw_.log_message(LogMsg::Info, name(), "Контроллер успешно активирован");
    next_state(&barrel_ctl::wait_add_liquid);
    active_ = false;
}

void barrel_ctl::on_deactivate() noexcept
{
    hw_.log_message(LogMsg::Info, name(), "Контроллер деактивирован");
}

void barrel_ctl::wait_add_liquid()
{
    if (!need_stop())
    {
        hw_.log_message(LogMsg::Info, name(), "Контроль активирован");
        next_state(&barrel_ctl::in_range);
        return;
    }

    if (need_liquid())
    {
        back_state_ = &barrel_ctl::wait_add_liquid;
        next_state(&barrel_ctl::add_liquid);
    }
}

void barrel_ctl::add_liquid()
{
    auto [ done, error ] = add_liquid_ctl_.set(add_liquid_value_);
    if (!dhresult::check(done, error, [this] { next_state(&barrel_ctl::add_liquid_error); }))
        return;
    next_state(back_state_);
}

void barrel_ctl::add_liquid_error()
{
    hw_.log_message(LogMsg::Error, name(), "Не удалось выполнить команду по доливке");
    next_state(back_state_);
}

// Мы в диапазоне заданной температуры
void barrel_ctl::in_range()
{
    if (need_stop())
    {
        next_state(&barrel_ctl::wait_add_liquid);
        return;
    }

    if (need_turn_on_heater())
    {
        hw_.log_message(LogMsg::Command, name(), "Включаем нагреватель");

        required_impact_ = &barrel_ctl::turn_on_heater;
        next_state(&barrel_ctl::turn_on_pump);

        return;
    }

    if (need_turn_on_cooler())
    {
        hw_.log_message(LogMsg::Command, name(), "Открываем клапан");

        required_impact_ = &barrel_ctl::turn_on_cooler;
        next_state(&barrel_ctl::turn_on_pump);

        return;
    }

    if (need_liquid())
    {
        back_state_ = &barrel_ctl::in_range;
        next_state(&barrel_ctl::add_liquid);
    }
}

void barrel_ctl::in_heater_range()
{
    if (temp_ >= target_temp_value() || need_stop() || !use_heater_)
    {
        hw_.log_message(LogMsg::Command, name(), "Выключаем нагреватель");

        next_state(&barrel_ctl::turn_off_heater);
        return;
    }

    if (need_liquid())
    {
        back_state_ = &barrel_ctl::in_heater_range;
        next_state(&barrel_ctl::add_liquid);
    }
}

void barrel_ctl::in_cooler_range()
{
    if (temp_ <= target_temp_value() || need_stop() || !use_cooler_)
    {
        hw_.log_message(LogMsg::Command, name(), "Закрываем клапан");

        next_state(&barrel_ctl::turn_off_cooler);
        return;
    }

    if (need_liquid())
    {
        back_state_ = &barrel_ctl::in_cooler_range;
        next_state(&barrel_ctl::add_liquid);
    }
}

void barrel_ctl::turn_on_pump()
{
    auto [ done, error ] = pump_ctl_.set(true);
    if (!dhresult::check(done, error, [this] { next_state(&barrel_ctl::turn_on_pump_error); }))
        return;
    next_state(required_impact_);
}

void barrel_ctl::turn_on_pump_error()
{
    hw_.log_message(LogMsg::Error, name(), "Не удалось включить насос");
    next_state(&barrel_ctl::wait_add_liquid);
}

void barrel_ctl::turn_on_heater()
{
    auto [ done, error ] = heater_ctl_.set(true);
    if (!dhresult::check(done, error, [this] { next_state(&barrel_ctl::turn_on_heater_error); }))
        return;
    next_state(&barrel_ctl::in_heater_range);
}

void barrel_ctl::turn_on_heater_error()
{
    hw_.log_message(LogMsg::Error, name(), "Не удалось включить нагреватель");
    next_state(&barrel_ctl::turn_off_pump);
}

void barrel_ctl::turn_on_cooler()
{
    auto [ done, error ] = cooler_ctl_.set(true);
    if (!dhresult::check(done, error, [this] { next_state(&barrel_ctl::turn_on_cooler_error); }))
        return;
    next_state(&barrel_ctl::in_cooler_range);
}

void barrel_ctl::turn_on_cooler_error()
{
    hw_.log_message(LogMsg::Error, name(), "Не удалось открыть клапан");
    next_state(&barrel_ctl::turn_off_pump);
}

void barrel_ctl::turn_off_pump()
{
    auto [ done, error ] = pump_ctl_.set(false);
    if (!dhresult::check(done, error, [this] { next_state(&barrel_ctl::turn_off_pump_error); }))
        return;
    next_state(&barrel_ctl::in_range);
}

void barrel_ctl::turn_off_pump_error()
{
    hw_.log_message(LogMsg::Error, name(), "Не удалось выключить насос");
    next_state(&barrel_ctl::in_range);
}

void barrel_ctl::turn_off_heater()
{
    auto [ done, error ] = heater_ctl_.set(false);
    if (!dhresult::check(done, error, [this] { next_state(&barrel_ctl::turn_off_heater_error); }))
        return;
    next_state(&barrel_ctl::turn_off_pump);
}

void barrel_ctl::turn_off_heater_error()
{
    hw_.log_message(LogMsg::Error, name(), "Не удалось выключить нагреватель");
    next_state(&barrel_ctl::in_range);
}

void barrel_ctl::turn_off_cooler()
{
    auto [ done, error ] = cooler_ctl_.set(false);
    if (!dhresult::check(done, error, [this] { next_state(&barrel_ctl::turn_off_cooler_error); }))
        return;
    next_state(&barrel_ctl::turn_off_pump);
}

void barrel_ctl::turn_off_cooler_error()
{
    hw_.log_message(LogMsg::Error, name(), "Не удалось закрыть клапан");
    next_state(&barrel_ctl::in_range);
}

bool barrel_ctl::need_turn_on_heater() const
{
    return ((temp_ < temp_min_) && use_heater_);
}

bool barrel_ctl::need_turn_on_cooler() const
{
    return ((temp_ > temp_max_) && use_cooler_);
}

float barrel_ctl::target_temp_value() const
{
    return temp_min_ + ((temp_max_ - temp_min_) / 2.0f);
}

bool barrel_ctl::need_stop()
{
    if (activate_)
    {
        active_ = activate_.value();
        activate_.reset();
    }
    return !active_;
}

bool barrel_ctl::need_liquid()
{
    if (!liquid_)
        return false;

    add_liquid_value_ = liquid_.value();
    liquid_.reset();

    return true;
}
