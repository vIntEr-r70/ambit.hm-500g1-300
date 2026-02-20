#pragma once

#include "eng/sibus/sibus.hpp"
#include "modbus-unit.hpp"

#include <eng/sibus/node.hpp>
#include <eng/timer.hpp>

#include <cstddef>

class frequency_converter final
    : public eng::sibus::node
    , public modbus_unit
{
    typedef void (frequency_converter::*read_handler_t)(readed_regs_t);
    std::unordered_map<std::size_t, read_handler_t> read_task_handlers_;

    eng::sibus::input_wire_id_t ictl_;

    constexpr static std::size_t kI{ 0 };
    constexpr static std::size_t kU{ 1 };
    constexpr static std::size_t kP{ 2 };

    enum pout
    {
        F,
        P,
        U_in,
        U_out,
        I,
        online,
        powered,
        damaged
    };
    std::array<eng::sibus::output_port_id_t, 8> p_out_;

    enum class estatus
    {
        idle,
        powered,
        damaged
    };
    std::optional<estatus> status_;

    std::array<std::uint16_t, 4> damages_;

    // Частота
    double F_;
    // Мощность
    double P_;
    // Напряжение
    double U_in_;
    double U_out_;
    // Ток
    double I_;

    std::array<double, 3> iup_{ 0.0, 0.0, 0.0 };

    double I_max_{ 0.0 };
    double P_max_{ 0.0 };

    struct priority_sets_t
    {
        double I{ 0.0 };
        double P{ 0.0 };
    };
    std::optional<priority_sets_t> priority_sets_;

public:

    frequency_converter(std::size_t);

private:

    void register_on_bus_done() override final;

private:

    void activate(eng::abc::pack);

    void deactivate();

private:

    void now_unit_online() override final;

    void connection_was_lost() override final;

    void read_task_done(std::size_t, readed_regs_t) override final;

private:

    void read_status_done(readed_regs_t);

    void read_F_done(readed_regs_t);

    void read_UUI_done(readed_regs_t);

    void read_P_done(readed_regs_t);

private:

    void write_sets();

    void write_priority_sets();
};

