#pragma once

#include "modbus-unit.hpp"

#include <eng/sibus/node.hpp>

#include <unordered_map>
#include <bitset>

class PR205_A1 final
    : public eng::sibus::node
    , public modbus_unit
{
    typedef void (PR205_A1::*handler_t)(readed_regs_t);

    std::unordered_map<std::size_t, handler_t> read_task_handlers_;

    struct sens_t
    {
        std::uint16_t value;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };

    std::array<sens_t, 2> fc_;
    std::array<sens_t, 1> dt_;
    std::array<sens_t, 2> dp_;

    struct valve_t
    {
        eng::sibus::input_wire_id_t ictl;
        eng::sibus::output_port_id_t port_out;
    };
    std::array<valve_t, 3> valves_;

    std::bitset<8> bs_0х4005_{ 0 };

    std::bitset<8> bs_0х4005_in_{ 0 };
    bool initialized_{ false };

public:

    PR205_A1(std::string_view, std::uint16_t);

private:

    void register_on_bus_done() override final;

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void connection_was_lost() override final;

private:

    void read_fc_done(readed_regs_t);

    void read_dt_done(readed_regs_t);

    void read_dp_done(readed_regs_t);

    void read_state_done(readed_regs_t);

private:

    void activate_valve(std::size_t);

    void deactivate_valve(std::size_t);

private:

    void open_valve(std::size_t);

    void close_valve(std::size_t);

    // void s_valve_opening(std::size_t);
    //
    // void s_valve_opened(std::size_t);
    //
    // void s_valve_closing(std::size_t);
    //
    // void s_valve_closed(std::size_t);
};
