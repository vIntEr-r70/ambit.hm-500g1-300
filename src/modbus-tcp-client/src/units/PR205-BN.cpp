#include "units/PR205-BN.hpp"

#include <eng/log.hpp>

#include <algorithm>

PR205_BN::PR205_BN(std::size_t id, std::string_view host, std::uint16_t port)
    : eng::sibus::node(std::format("PR205-B{}", id))
    , modbus_unit(host, port)
{
    std::size_t idx;

    idx = modbus_unit::add_read_task(0x4002, 4, 1000);
    read_task_handlers_[idx] = &PR205_BN::read_dt_done;
    for (std::size_t i = 0; i < dt_.size(); ++i)
        dt_[i].port_id = add_output_port(std::format("DT{}", i + 1));

    idx = modbus_unit::add_read_task(0x4006, fc_.size(), 1000);
    read_task_handlers_[idx] = &PR205_BN::read_fc_done;
    for (std::size_t i = 0; i < fc_.size(); ++i)
        fc_[i].port_id = add_output_port(std::format("FC{}", i + 1));

    idx = modbus_unit::add_read_task(0x4001, 1, 200);
    read_task_handlers_[idx] = &PR205_BN::read_state_done;

    for (std::size_t i = 0; i < pumps_.size(); ++i)
    {
        node::add_input_port(std::format("H{}", i + 1), [this, i](eng::abc::pack args) {
            start_stop_pump(i, eng::abc::get<bool>(args));
        });
        pumps_[i].port_id = node::add_output_port(std::format("H{}", i + 1));
    }

    for (std::size_t i = 0; i < valves_.size(); ++i)
    {
        node::add_input_port(std::format("VA{}", i + 1), [this, i](eng::abc::pack args) {
            open_close_valve(i, eng::abc::get<bool>(args));
        });
        valves_[i].port_id = node::add_output_port(std::format("VA{}", i + 1));
    }
}

void PR205_BN::read_task_done(std::size_t idx, readed_regs_t regs)
{
    (this->*read_task_handlers_[idx])(regs);
}

void PR205_BN::connection_was_lost()
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

    std::ranges::for_each(pumps_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });

    std::ranges::for_each(valves_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });
}

void PR205_BN::read_fc_done(readed_regs_t regs)
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

        eng::log::info("{}.FC{} = {}", name(), i + 1, item.value);
    }
}

void PR205_BN::read_dt_done(readed_regs_t regs)
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
    }
}

void PR205_BN::read_state_done(readed_regs_t regs)
{
    decltype(bs_0х4001_) bitset{ regs[0] };

    for (std::size_t i = 0; i < pumps_.size(); ++i)
    {
        auto &item = pumps_[i];

        if (item.value == bitset[i] && item.initialized)
            continue;

        item.value = bitset[i];
        item.initialized = true;

        node::set_port_value(item.port_id, { item.value });
    }

    bitset >>= 3;

    for (std::size_t i = 0; i < valves_.size(); ++i)
    {
        auto &item = valves_[i];

        if (item.value == bitset[i] && item.initialized)
            continue;

        item.value = bitset[i];
        item.initialized = true;

        node::set_port_value(item.port_id, { item.value });
    }
}

/////////////////////////////////////////////////////////////////////////

void PR205_BN::start_stop_pump(std::size_t idx, bool value)
{
    bs_0х4001_.set(idx + 0, value);
    modbus_unit::write_single(0x4001, bs_0х4001_.to_ulong());
}

void PR205_BN::open_close_valve(std::size_t idx, bool value)
{
    bs_0х4001_.set(idx + 3, value);
    modbus_unit::write_single(0x4001, bs_0х4001_.to_ulong());
}

