#pragma once

#include <eng/sibus/node.hpp>

class barrel_ctl final
    : public eng::sibus::node
{
    struct
    {
        bool use_heater{ false };
        bool use_cooler{ false };

        bool pump_full_time{ false };

        double dt_min{ 20.0 };
        double dt_max{ 80.0 };

    } cfg_;

    std::optional<double> current_temperature_;

    eng::sibus::output_port_id_t pump_;
    eng::sibus::output_port_id_t heater_;
    eng::sibus::output_port_id_t cooler_;
    eng::sibus::output_port_id_t active_;

    void (barrel_ctl::*in_range_)();

public:

    barrel_ctl(std::string_view);

private:

    void deactivate();

private:

    void in_temperature_range();

    void in_heater_range();

    void in_cooler_range();

private:

    bool need_turn_on_heater() const noexcept {
        return (*current_temperature_ < cfg_.dt_min) && cfg_.use_heater;
    }

    bool need_turn_on_cooler() const noexcept {
        return (*current_temperature_ > cfg_.dt_max) && cfg_.use_cooler;
    }

    double middle_temp_value() const noexcept {
        return *current_temperature_ + ((cfg_.dt_max - cfg_.dt_min) * 0.5);
    }

    bool need_stay_in_heater_range() const noexcept {
        return (*current_temperature_ < middle_temp_value()) && cfg_.use_heater;
    }

    bool need_stay_in_cooler_range() const noexcept {
        return (*current_temperature_ > middle_temp_value()) && cfg_.use_cooler;
    }
};

