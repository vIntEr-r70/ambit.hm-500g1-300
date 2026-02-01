#pragma once

#include "modbus-unit.hpp"

#include <eng/sibus/node.hpp>

#include <bitset>
#include <unordered_map>

class PR205_BN final
    : public eng::sibus::node
    , public modbus_unit
{
    typedef void (PR205_BN::*read_handler_t)(readed_regs_t);
    typedef void (PR205_BN::*write_handler_t)();

    std::unordered_map<std::size_t, read_handler_t> read_task_handlers_;
    std::unordered_map<std::size_t, write_handler_t> write_task_handlers_;

    struct sens_t
    {
        std::uint16_t value;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };

    std::array<sens_t, 4> fc_;
    std::array<sens_t, 4> dt_;

    struct pump_t
    {
        void(PR205_BN::*state)(std::size_t);
        eng::sibus::input_wire_id_t ictl;
    };
    std::array<pump_t, 2> pumps_;

    struct valve_t
    {
        void(PR205_BN::*state)(std::size_t);
        eng::sibus::input_wire_id_t ictl;
    };
    std::array<valve_t, 2> valves_;

    std::bitset<8> bs_0х4001_{ 0 };

public:

    PR205_BN(std::size_t, std::string_view, std::uint16_t);

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void write_task_done(std::size_t) override final;

    void connection_was_lost() override final;

private:

    void read_fc_done(readed_regs_t);

    void read_dt_done(readed_regs_t);

    void read_state_done(readed_regs_t);

private:

    void s_pump_opening(std::size_t);

    void s_pump_opened(std::size_t);

    void s_pump_closing(std::size_t);

    void s_pump_closed(std::size_t);

private:

    void s_valve_opening(std::size_t);

    void s_valve_opened(std::size_t);

    void s_valve_closing(std::size_t);

    void s_valve_closed(std::size_t);
};

