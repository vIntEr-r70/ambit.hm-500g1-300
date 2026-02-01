#include "units/PR205-A14.hpp"

#include <eng/log.hpp>

#include <algorithm>

PR205_A14::PR205_A14(std::string_view host, std::uint16_t port)
    : eng::sibus::node("PR205-A14")
    , modbus_unit(host, port)
{
    std::size_t idx;

    idx = modbus_unit::add_read_task(0x4012, dt_.size(), 1000);
    read_task_handlers_[idx] = &PR205_A14::read_dt_done;
    for (std::size_t i = 0; i < dt_.size(); ++i)
        dt_[i].port_id = add_output_port(std::format("DT{}", i + 1));

    idx = modbus_unit::add_read_task(0x4014, dp_.size(), 1000);
    read_task_handlers_[idx] = &PR205_A14::read_dp_done;
    for (std::size_t i = 0; i < dp_.size(); ++i)
        dp_[i].port_id = add_output_port(std::format("DP{}", i + 1));
}

void PR205_A14::read_task_done(std::size_t idx, readed_regs_t regs)
{
    (this->*read_task_handlers_[idx])(regs);
}

void PR205_A14::connection_was_lost()
{
    std::ranges::for_each(dt_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });

    std::ranges::for_each(dp_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });
}

void PR205_A14::read_dt_done(readed_regs_t regs)
{
    for (std::size_t i = 0; i < dt_.size(); ++i)
    {
        auto &item = dt_[i];

        if (item.value == regs[i] && item.initialized)
            continue;

        item.value = regs[i];
        item.initialized = true;

        double v = static_cast<std::int16_t>(item.value) * 0.1;
        node::set_port_value(item.port_id, { v });

        // eng::log::info("{}: DT{} = {}", name(), i + 1, v);
    }
}

void PR205_A14::read_dp_done(readed_regs_t regs)
{
    for (std::size_t i = 0; i < dp_.size(); ++i)
    {
        auto &item = dp_[i];

        if (item.value == regs[i] && item.initialized)
            continue;

        item.value = regs[i];
        item.initialized = true;

        double v = static_cast<std::int16_t>(item.value) * 0.1;
        node::set_port_value(item.port_id, { v });

        // eng::log::info("{}: DP{} = {}", name(), i + 1, v);
    }
}
