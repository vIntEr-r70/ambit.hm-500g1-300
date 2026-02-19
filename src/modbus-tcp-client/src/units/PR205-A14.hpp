#pragma once

#include "modbus-unit.hpp"

#include <eng/sibus/node.hpp>

#include <unordered_map>
#include <bitset>

class PR205_A14 final
    : public eng::sibus::node
    , public modbus_unit
{
    typedef void (PR205_A14::*handler_t)(readed_regs_t);

    std::unordered_map<std::size_t, handler_t> read_task_handlers_;

    template <typename T>
    struct sens_t
    {
        T value;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };

    std::array<sens_t<std::uint16_t>, 2> dt_;
    std::array<sens_t<std::uint16_t>, 2> dp_;

    std::array<sens_t<std::uint8_t>, 3> vp_;
    std::bitset<8> bs_0х4001_{ 0 };

public:

    PR205_A14(std::string_view, std::uint16_t);

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void connection_was_lost() override final;

private:

    void read_dt_done(readed_regs_t);

    void read_dp_done(readed_regs_t);

    void read_vp_done(readed_regs_t);

    void switch_vp(std::size_t, bool);
};
