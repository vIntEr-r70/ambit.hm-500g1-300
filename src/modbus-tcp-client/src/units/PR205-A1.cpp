#include "units/PR205-A1.hpp"
#include "eng/sibus/client.hpp"
#include "modbus-unit.hpp"

#include <eng/log.hpp>

#include <algorithm>
#include <initializer_list>
#include <string_view>
#include <span>

PR205_A1::PR205_A1(std::string_view host, std::uint16_t port)
    : eng::sibus::node("PR205-A1")
    , modbus_unit(host, port)
{
    std::size_t idx;

    idx = modbus_unit::add_read_task(0x4001, fc_.size(), 1000);
    read_task_handlers_[idx] = &PR205_A1::read_fc_done;

    idx = modbus_unit::add_read_task(0x4006, dt_.size(), 1000);
    read_task_handlers_[idx] = &PR205_A1::read_dt_done;

    idx = modbus_unit::add_read_task(0x4008, dp_.size(), 1000);
    read_task_handlers_[idx] = &PR205_A1::read_dp_done;

    idx = modbus_unit::add_read_task(0x4005, 1, 200);
    read_task_handlers_[idx] = &PR205_A1::read_state_done;

    using span_list = std::initializer_list<std::pair<std::span<sens_t<std::uint16_t>>, std::string_view>>;
    for (auto [ sens, key ] : span_list{ { std::span{fc_}, "FC" }, { std::span{dp_}, "DP" }, { std::span{dt_}, "DT" } })
    {
        for (std::size_t i = 0; i < sens.size(); ++i)
            sens[i].port_id = node::add_output_port(std::format("{}{}", key, i + 1));
    }

    for (std::size_t i = 0; i < valves_.size(); ++i)
    {
        node::add_input_port_unsafe(std::format("A{}", i + 1), [this, i](eng::abc::pack args)
        {
            eng::log::info("A{} = {}", i, args.size());
            open_close_valve(i, args ? eng::abc::get<bool>(args) : false);
        });
        valves_[i].port_id = node::add_output_port(std::format("A{}", i + 1));
    }
}

void PR205_A1::read_task_done(std::size_t idx, readed_regs_t regs)
{
    (this->*read_task_handlers_[idx])(regs);
}

void PR205_A1::connection_was_lost()
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

    std::ranges::for_each(dp_, [this](auto &item)
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

void PR205_A1::read_fc_done(readed_regs_t regs)
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
    }
}

void PR205_A1::read_dt_done(readed_regs_t regs)
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

void PR205_A1::read_dp_done(readed_regs_t regs)
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
    }
}

void PR205_A1::read_state_done(readed_regs_t regs)
{
    decltype(bs_0х4005_) bitset{ regs[0] };

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

void PR205_A1::open_close_valve(std::size_t idx, bool value)
{
    eng::log::info("{}: {}: idx = {}, value = {}", name(), __func__, idx, value);

    bs_0х4005_.set(idx, value);
    modbus_unit::write_single(0x4005, bs_0х4005_.to_ulong());
}

