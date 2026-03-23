#pragma once

#include <eng/modbus/unit.hpp>
#include <eng/sibus/node.hpp>

#include <unordered_map>
#include <array>

class PR200 final
    : public eng::sibus::node
    , public eng::modbus::unit
{
    typedef void (PR200::*handler_t)(readed_regs_t);

    std::unordered_map<std::size_t, handler_t> read_task_handlers_;

    template <typename T>
    struct sens_t
    {
        T value;
        std::size_t idx;
        bool initialized{ false };
        eng::sibus::output_port_id_t port_id;
    };

    std::array<sens_t<std::uint16_t>, 2> fc_;
    std::array<sens_t<std::uint16_t>, 2> dt_;
    std::array<sens_t<bool>, 1> output_;

public:

    PR200(std::uint8_t);

private:

    void register_on_bus_done() override final;

private:

    void read_task_done(std::size_t, readed_regs_t) override final;

    void connection_was_lost() override final;

private:

    void read_fc_done(readed_regs_t);

    void read_dt_done(readed_regs_t);

    void read_output_state_done(readed_regs_t);
};

