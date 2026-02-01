#pragma once

#include "modbus-unit.hpp"

#include <eng/sibus/node.hpp>

#include <unordered_map>

class PR205_A2 final
    : public eng::sibus::node
    , public modbus_unit
{
    typedef void (PR205_A2::*handler_t)(readed_regs_t);

    std::unordered_map<std::size_t, handler_t> read_task_handlers_;

    struct sens_t
    {
        std::uint16_t value;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };

    std::array<sens_t, 8> fc_;
    std::array<sens_t, 7> dt_;
    std::array<sens_t, 1> dp_;

public:

    PR205_A2(std::string_view, std::uint16_t);

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void connection_was_lost() override final;

private:

    void read_fc_done(readed_regs_t);

    void read_dt_done(readed_regs_t);

    void read_dp_done(readed_regs_t);

};
