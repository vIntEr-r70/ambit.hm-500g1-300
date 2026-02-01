#include "units/PR205-BN.hpp"

#include <eng/log.hpp>

#include <algorithm>

PR205_BN::PR205_BN(std::size_t id, std::string_view host, std::uint16_t port)
    : eng::sibus::node(std::format("PR205-B{}", id))
    , modbus_unit(host, port)
{
    std::size_t idx;

    idx = modbus_unit::add_read_task(0x4012, fc_.size(), 1000);
    read_task_handlers_[idx] = &PR205_BN::read_fc_done;
    for (std::size_t i = 0; i < fc_.size(); ++i)
        fc_[i].port_id = add_output_port(std::format("FC{}", i + 1));

    idx = modbus_unit::add_read_task(0x400E, dt_.size(), 1000);
    read_task_handlers_[idx] = &PR205_BN::read_dt_done;
    for (std::size_t i = 0; i < dt_.size(); ++i)
        dt_[i].port_id = add_output_port(std::format("DT{}", i + 1));

    idx = modbus_unit::add_read_task(0x4001, 1, 200);
    read_task_handlers_[idx] = &PR205_BN::read_state_done;

    for (std::size_t i = 0; i < pumps_.size(); ++i)
    {
        pumps_[i].ictl = node::add_input_wire(std::format("H{}", i + 1));
        pumps_[i].state = &PR205_BN::s_pump_closed;
        node::set_activate_handler(pumps_[i].ictl, [this, i](eng::abc::pack) {
            (this->*pumps_[i].state)(i);
        });
        node::set_deactivate_handler(pumps_[i].ictl, [this, i] {
            (this->*pumps_[i].state)(i);
        });

        pumps_[i].port_out = node::add_output_port(std::format("H{}", i + 1));
    }

    for (std::size_t i = 0; i < valves_.size(); ++i)
    {
        valves_[i].ictl = node::add_input_wire(std::format("A{}", i + 1));
        valves_[i].state = &PR205_BN::s_valve_closed;
        node::set_activate_handler(valves_[i].ictl, [this, i](eng::abc::pack) {
            (this->*valves_[i].state)(i);
        });
        node::set_deactivate_handler(valves_[i].ictl, [this, i] {
            (this->*valves_[i].state)(i);
        });
    }
}

void PR205_BN::read_task_done(std::size_t idx, readed_regs_t regs)
{
    (this->*read_task_handlers_[idx])(regs);
}

void PR205_BN::write_task_done(std::size_t)
{
    eng::log::info("{}: write_task_done", name());
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

        eng::log::info("{}: FC{} = {}", name(), i + 1, v);
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

        eng::log::info("{}: DT{} = {}", name(), i + 1, v);
    }
}

void PR205_BN::read_state_done(readed_regs_t regs)
{
    decltype(bs_0х4001_) bitset{ regs[0] };

    if (bs_0х4001_ == bitset) return;
    bs_0х4001_ = bitset;

    for (std::size_t i = 0; i < valves_.size(); ++i)
    {
        auto &valve = valves_[i];
        (this->*valve.state)(i);
    };

    for (std::size_t i = 0; i < pumps_.size(); ++i)
    {
        auto &pump = pumps_[i];
        (this->*pump.state)(i);

        node::set_port_value(pump.port_out, { bs_0х4001_.test(i) });
    };
}

/////////////////////////////////////////////////////////////////////////

void PR205_BN::s_valve_opening(std::size_t idx)
{
    eng::log::info("{}: {}", name(), __func__);

    // Еще не открыта
    if (!bs_0х4001_.test(idx + 2) && modbus_unit::is_online())
    {
        eng::log::info("\twaiting");
        return;
    }

    auto &valve = valves_[idx];

    if (!bs_0х4001_.test(idx + 2))
    {
        eng::log::info("\tswitch to closed");
        valve.state = &PR205_BN::s_valve_closed;
        node::deactivated(valve.ictl);
        return;
    }

    valve.state = &PR205_BN::s_valve_opened;
    node::activated(valve.ictl);
}

void PR205_BN::s_valve_opened(std::size_t idx)
{
    eng::log::info("{}: {}", name(), __func__);

    auto &valve = valves_[idx];

    // Все еще открыта
    if (bs_0х4001_.test(idx + 2) && node::is_active(valve.ictl))
        return;

    if (!modbus_unit::is_online())
    {
        node::activated(valve.ictl);
        return;
    }

    if (!bs_0х4001_.test(idx + 2))
    {
        node::deactivated(valve.ictl);
        valve.state = &PR205_BN::s_valve_closed;
        return;
    }

    // Отправляем команду закрытия
    auto bitset = bs_0х4001_;
    bitset.set(idx + 2, false);
    modbus_unit::write_single(0x4001, bitset.to_ulong());
    valve.state = &PR205_BN::s_valve_closing;
}

void PR205_BN::s_valve_closing(std::size_t idx)
{
    eng::log::info("{}: {}", name(), __func__);

    // Еще не закрыта
    if (bs_0х4001_.test(idx + 2) && modbus_unit::is_online())
        return;

    auto &valve = valves_[idx];

    if (bs_0х4001_.test(idx + 2))
    {
        valve.state = &PR205_BN::s_valve_opened;
        node::activated(valve.ictl);
        return;
    }

    // Закрыта
    valve.state = &PR205_BN::s_valve_closed;

    node::deactivated(valve.ictl);
}

void PR205_BN::s_valve_closed(std::size_t idx)
{
    eng::log::info("{}: {}", name(), __func__);

    auto &valve = valves_[idx];

    // Все еще закрыта
    if (!bs_0х4001_.test(idx + 2) && !node::is_active(valve.ictl))
        return;

    if (!modbus_unit::is_online())
    {
        node::deactivated(valve.ictl);
        return;
    }

    if (bs_0х4001_.test(idx + 2))
    {
        node::activated(valve.ictl);
        valve.state = &PR205_BN::s_valve_opened;
        return;
    }

    // Отправляем команду открытия
    auto bitset = bs_0х4001_;
    bitset.set(idx + 2, true);
    modbus_unit::write_single(0x4001, bitset.to_ulong());
    valve.state = &PR205_BN::s_valve_opening;
}

void PR205_BN::s_pump_opening(std::size_t idx)
{
    eng::log::info("{}: {}", name(), __func__);

    // Еще не открыта
    if (!bs_0х4001_.test(idx) && modbus_unit::is_online())
        return;

    auto &pump = pumps_[idx];

    if (!bs_0х4001_.test(idx))
    {
        pump.state = &PR205_BN::s_pump_closed;
        node::deactivated(pump.ictl);
        return;
    }

    pump.state = &PR205_BN::s_pump_opened;
    node::activated(pump.ictl);
}

void PR205_BN::s_pump_opened(std::size_t idx)
{
    eng::log::info("{}: {}", name(), __func__);

    auto &pump = pumps_[idx];

    // Все еще открыта
    if (bs_0х4001_.test(idx) && node::is_active(pump.ictl))
        return;

    if (!modbus_unit::is_online())
    {
        node::activated(pump.ictl);
        return;
    }

    if (!bs_0х4001_.test(idx))
    {
        node::deactivated(pump.ictl);
        pump.state = &PR205_BN::s_pump_closed;
        return;
    }

    // Отправляем команду закрытия
    auto bitset = bs_0х4001_;
    bitset.set(idx, false);
    modbus_unit::write_single(0x4001, bitset.to_ulong());
    pump.state = &PR205_BN::s_pump_closing;
}

void PR205_BN::s_pump_closing(std::size_t idx)
{
    eng::log::info("{}: {}", name(), __func__);

    // Еще не закрыта
    if (bs_0х4001_.test(idx) && modbus_unit::is_online())
        return;

    auto &pump = pumps_[idx];

    if (bs_0х4001_.test(idx))
    {
        pump.state = &PR205_BN::s_pump_opened;
        node::activated(pump.ictl);
        return;
    }

    // Закрыта
    pump.state = &PR205_BN::s_pump_closed;

    node::deactivated(pump.ictl);
}

void PR205_BN::s_pump_closed(std::size_t idx)
{
    eng::log::info("{}: {}", name(), __func__);

    auto &pump = pumps_[idx];

    // Все еще закрыта
    if (!bs_0х4001_.test(idx) && !node::is_active(pump.ictl))
        return;

    if (!modbus_unit::is_online())
    {
        node::deactivated(pump.ictl);
        return;
    }

    if (bs_0х4001_.test(idx))
    {
        node::activated(pump.ictl);
        pump.state = &PR205_BN::s_pump_opened;
        return;
    }

    eng::log::info("{}: {}", name(), __func__);

    // Отправляем команду открытия
    auto bitset = bs_0х4001_;
    bitset.set(idx, true);
    modbus_unit::write_single(0x4001, bitset.to_ulong());
    pump.state = &PR205_BN::s_pump_opening;
}


