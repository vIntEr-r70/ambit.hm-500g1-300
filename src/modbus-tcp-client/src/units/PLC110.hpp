#pragma once

#include <eng/modbus/unit.hpp>
#include <eng/sibus/node.hpp>

#include <unordered_map>
#include <bitset>
#include <array>

class PLC110 final
    : public eng::sibus::node
    , public eng::modbus::unit
{
    typedef void (PLC110::*handler_t)(readed_regs_t);

    std::unordered_map<std::size_t, handler_t> read_task_handlers_;

    template <typename T>
    struct sens_t
    {
        T value;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };

    sens_t<std::int32_t> spin_;

    std::array<sens_t<bool>, 32> plc_;
    std::array<std::array<sens_t<bool>, 8>, 2> mk_;

    std::bitset<20> outputs_;
    std::array<std::bitset<8>, 2> mk_outputs_;

public:

    PLC110(std::uint8_t);

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void connection_was_lost() override final;

    void now_unit_online() override final;

private:

    void read_spin_done(readed_regs_t);

    void read_plc_done(readed_regs_t);

    void read_mk_1_done(readed_regs_t);

    void read_mk_2_done(readed_regs_t);

    void read_mk_done(std::size_t, readed_regs_t);

private:

    void write_outputs();

    void write_mk_outputs(std::size_t);

    void update_outputs();

    void update_mk_outputs(std::size_t);
};
