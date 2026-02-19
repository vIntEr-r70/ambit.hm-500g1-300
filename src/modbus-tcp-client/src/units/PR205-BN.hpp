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

    template <typename T>
    struct sens_t
    {
        std::uint16_t value;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };

    std::array<sens_t<std::uint16_t>, 4> fc_;
    std::array<sens_t<std::uint16_t>, 4> dt_;

    std::array<sens_t<bool>, 2> pumps_;
    std::array<sens_t<bool>, 2> valves_;
    std::array<sens_t<bool>, 1> heater_;

    std::bitset<8> bs_0х4001_{ 0 };

public:

    PR205_BN(std::size_t, std::string_view, std::uint16_t);

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void connection_was_lost() override final;

private:

    void read_fc_done(readed_regs_t);

    void read_dt_done(readed_regs_t);

    void read_state_done(readed_regs_t);

private:

    void start_stop_pump(std::size_t, bool);

    void open_close_valve(std::size_t, bool);

    void on_off_heater(std::size_t, bool);
};

