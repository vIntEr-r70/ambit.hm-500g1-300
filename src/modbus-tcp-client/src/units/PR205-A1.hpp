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

    template <typename T>
    struct sens_t
    {
        T value;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };

    std::array<sens_t<std::uint16_t>, 2> fc_;
    std::array<sens_t<std::uint16_t>, 1> dt_;
    std::array<sens_t<std::uint16_t>, 2> dp_;

    std::array<sens_t<bool>, 3> valves_;

    std::bitset<8> bs_0х4005_{ 0 };

public:

    PR205_A1(std::string_view, std::uint16_t);

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void connection_was_lost() override final;

private:

    void read_fc_done(readed_regs_t);

    void read_dt_done(readed_regs_t);

    void read_dp_done(readed_regs_t);

    void read_state_done(readed_regs_t);

private:

    void open_close_valve(std::size_t, bool);
};
