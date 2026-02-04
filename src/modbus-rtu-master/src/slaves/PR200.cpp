#include "slaves/PR200.hpp"

#include <eng/log.hpp>

#include <algorithm>

PR200::PR200(std::uint8_t id)
    : eng::sibus::node("PR200")
    , modbus_unit(id)
{
    std::size_t idx;

    idx = modbus_unit::add_read_task(0x0200, fc_.size(), 1000);
    read_task_handlers_[idx] = &PR200::read_fc_done;
    for (std::size_t i = 0; i < fc_.size(); ++i)
        fc_[i].port_id = add_output_port(std::format("FC{}", i + 1));

    idx = modbus_unit::add_read_task(0x0202, dt_.size(), 1000);
    read_task_handlers_[idx] = &PR200::read_dt_done;
    for (std::size_t i = 0; i < dt_.size(); ++i)
        dt_[i].port_id = add_output_port(std::format("DT{}", i + 1));
}

void PR200::read_task_done(std::size_t idx, readed_regs_t regs)
{
    (this->*read_task_handlers_[idx])(regs);
}

void PR200::connection_was_lost()
{
    std::ranges::for_each(fc_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });

    std::ranges::for_each(dt_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });
}

void PR200::read_fc_done(readed_regs_t regs)
{
    for (std::size_t i = 0; i < fc_.size(); ++i)
    {
        auto &item = fc_[i];

        if (item.value == regs[i] && item.initialized)
            continue;

        item.value = regs[i];
        item.initialized = true;

        double v = static_cast<std::int16_t>(item.value) * 0.1;
        node::set_port_value(item.port_id, { v });

        // eng::log::info("{}: FC{} = {}", name(), i + 1, v);
    }
}

void PR200::read_dt_done(readed_regs_t regs)
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

