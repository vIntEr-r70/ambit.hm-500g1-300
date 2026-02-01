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

    struct
    {
        std::size_t idx;
        void (frequency_converter::*handler)(bool);

        bool in_proc() const { return handler != nullptr; }

    } write_task_handler_;

    std::array<std::uint16_t, 4> damages_;

    void (frequency_converter::*hw_state_)(std::uint16_t);

    // Частота
    double F_;
    // Мощность
    double P_;
    // Напряжение
    double U_in_;
    double U_out_;
    // Ток
    double I_;

    std::array<double, 3> iup_;
    double I_max_{ 0.0 };

public:

    frequency_converter(std::size_t);

private:

    void activate(eng::abc::pack);

    void deactivate();

private:

    void s_idle(std::uint16_t);

    void s_powered(std::uint16_t);

    void s_damaged(std::uint16_t);

private:

    void now_unit_online() override final;

    void connection_was_lost() override final;

    void read_task_done(std::size_t, readed_regs_t) override final;

    void write_task_done(std::size_t) override final;

private:

    void w_start(bool);

    void w_stop(bool);

private:

    void read_status_done(readed_regs_t);

    void read_F_done(readed_regs_t);

    void read_UUI_done(readed_regs_t);

    void read_P_done(readed_regs_t);
};

