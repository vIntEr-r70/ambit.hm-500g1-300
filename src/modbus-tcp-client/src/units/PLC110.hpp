#pragma once

#include "modbus-unit.hpp"

#include <eng/sibus/node.hpp>

#include <unordered_map>
#include <bitset>

class PLC110 final
    : public eng::sibus::node
    , public modbus_unit
{
    typedef void (PLC110::*handler_t)(readed_regs_t);

    std::unordered_map<std::size_t, handler_t> read_task_handlers_;

    struct spin_value_t
    {
        std::int32_t value;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };
    spin_value_t spin_;

    template <std::size_t BITS>
    struct bits_value_t
    {
        std::bitset<BITS> bitset;
        bool initialized{ false };
        std::array<eng::sibus::output_port_id_t, BITS> ports_id;
    };
    bits_value_t<32> bits_;

    std::array<bits_value_t<8>, 2> mk_bits_;

    std::bitset<20> outputs_;
    std::array<std::bitset<8>, 2> mk_outputs_;

public:

    PLC110(std::string_view, std::uint16_t);

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void connection_was_lost() override final;

private:

    void read_spin_done(readed_regs_t);

    void read_bits_done(readed_regs_t);

    void read_mk_1_done(readed_regs_t);

    void read_mk_2_done(readed_regs_t);

    void read_mk_done(std::size_t, readed_regs_t);

private:

    void write_outputs();

    void write_mk_outputs(std::size_t);
};
