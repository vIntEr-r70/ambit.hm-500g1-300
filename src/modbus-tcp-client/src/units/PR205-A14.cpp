#include "units/PR205-A14.hpp"

#include <eng/log.hpp>

#include <algorithm>

PR205_A14::PR205_A14(std::uint8_t slave_id)
    : eng::sibus::node("PR205-A14")
    , eng::modbus::unit(slave_id)
{
    std::size_t idx;

    idx = unit::add_read_task(0x4002, dt_.size(), 1000);
    read_task_handlers_[idx] = &PR205_A14::read_dt_done;
    for (std::size_t i = 0; i < dt_.size(); ++i)
        dt_[i].port_id = add_output_port(std::format("DT{}", i + 1));

    idx = unit::add_read_task(0x4004, dp_.size(), 1000);
    read_task_handlers_[idx] = &PR205_A14::read_dp_done;
    for (std::size_t i = 0; i < dp_.size(); ++i)
        dp_[i].port_id = add_output_port(std::format("DP{}", i + 1));

    idx = unit::add_read_task(0x400A, 1, 200);
    read_task_handlers_[idx] = &PR205_A14::read_va_done;

    idx = unit::add_read_task(0x4000, 1, 200);
    read_task_handlers_[idx] = &PR205_A14::read_sens_done;

    idx = unit::add_read_task(0x4001, 1, 200);
    read_task_handlers_[idx] = &PR205_A14::read_pump_done;

    static constexpr std::size_t sens_idx_map[] = { 3, 4, 1, 2, 5 };
    for (std::size_t i = 0; i < sens_.size(); ++i)
        sens_[i].port_id = node::add_output_port(std::format("P{}", sens_idx_map[i]));

    for (std::size_t i = 0; i < va_.size(); ++i)
    {
        // Входящее состояние задвижки
        node::add_input_port(std::format("VA{}", i + 1), [this, i](eng::abc::pack args)
        {
            switch_va(i, eng::abc::get<bool>(args));
        });
        // Реальное состояние задвижки
        va_[i].port_id = node::add_output_port(std::format("VA{}", i + 1));
    }

    node::add_input_port("H", [this](eng::abc::pack args)
    {
        switch_pump(eng::abc::get<bool>(args));
    });
    pump_.port_id = node::add_output_port("H");
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

    std::ranges::for_each(sens_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });

    std::ranges::for_each(va_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });

    pump_.initialized = false;
    node::set_port_value(pump_.port_id, { });
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
    }
}

void PR205_A14::read_sens_done(readed_regs_t regs)
{
    std::bitset<5> bitset{ regs[0] };

    for (std::size_t i = 0; i < sens_.size(); ++i)
    {
        auto &item = sens_[i];

        if (item.value == bitset[i] && item.initialized)
            continue;

        item.value = bitset[i];
        item.initialized = true;

        node::set_port_value(item.port_id, { item.value });
    }
}

void PR205_A14::read_pump_done(readed_regs_t regs)
{
    std::bitset<1> bitset{ regs[0] };

    if (pump_.value == bitset.test(0) && pump_.initialized)
        return;

    pump_.value = bitset.test(0);
    pump_.initialized = true;

    node::set_port_value(pump_.port_id, { pump_.value });
}

void PR205_A14::read_va_done(readed_regs_t regs)
{
    for (std::size_t i = 0; i < va_.size(); ++i)
    {
        auto &item = va_[i];

        std::bitset<2> value(regs[0] >> (i * 2));

        if (item.value == value.to_ulong() && item.initialized)
            continue;

        item.value = value.to_ulong();
        item.initialized = true;

        if (value.test(0) == value.test(1))
            node::set_port_value(item.port_id, { });
        else
            node::set_port_value(item.port_id, { value.test(1) });
    }
}

void PR205_A14::switch_va(std::size_t idx, bool value)
{
    idx = (idx * 2) + 1;
    bs_h4001_.set(idx + 0, value);
    bs_h4001_.set(idx + 1, !value);
    unit::write_single(0x4001, bs_h4001_.to_ulong());
}

void PR205_A14::switch_pump(bool value)
{
    bs_h4001_.set(0, value);
    unit::write_single(0x4001, bs_h4001_.to_ulong());
}

