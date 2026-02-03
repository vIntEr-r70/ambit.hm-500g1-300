#include "units/PR205-A1.hpp"
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

    using span_list = std::initializer_list<std::pair<std::span<sens_t>, std::string_view>>;
    for (auto [ sens, key ] : span_list{ { std::span{fc_}, "FC" }, { std::span{dp_}, "DP" }, { std::span{dt_}, "DT" } })
    {
        for (std::size_t i = 0; i < sens.size(); ++i)
            sens[i].port_id = node::add_output_port(std::format("{}{}", key, i + 1));
    }

    for (std::size_t i = 0; i < valves_.size(); ++i)
    {
        valves_[i].ictl = node::add_input_wire(std::format("A{}", i + 1));
        // valves_[i].state = &PR205_A1::s_valve_closed;
        node::set_activate_handler(valves_[i].ictl, [this, i](eng::abc::pack)
        {
            // (this->*valves_[i].state)(i);
            open_valve(i);
        });
        node::set_deactivate_handler(valves_[i].ictl, [this, i]
        {
            // (this->*valves_[i].state)(i);
            close_valve(i);
        });

        valves_[i].port_out = node::add_output_port(std::format("A{}", i + 1));
    }
}

void PR205_A1::register_on_bus_done()
{
    for (std::size_t i = 0; i < valves_.size(); ++i)
        node::set_ready(valves_[i].ictl);
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

    for (std::size_t i = 0; i < valves_.size(); ++i)
    {
        // auto &valve = valves_[i];
        // (this->*valve.state)(i);
    };
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

    if (bs_0х4005_ == bitset) return;
    bs_0х4005_ = bitset;

    for (std::size_t i = 0; i < valves_.size(); ++i)
    {
        auto &valve = valves_[i];
        // (this->*valve.state)(i);
        node::set_port_value(valve.port_out, { bs_0х4005_.test(i) });
    };
}

void PR205_A1::open_valve(std::size_t idx)
{
    auto bitset = bs_0х4005_;
    bitset.set(idx, true);
    modbus_unit::write_single(0x4005, bitset.to_ulong());
}

void PR205_A1::close_valve(std::size_t idx)
{
    auto bitset = bs_0х4005_;
    bitset.set(idx, false);
    modbus_unit::write_single(0x4005, bitset.to_ulong());
    node::set_ready(valves_[idx].ictl);
}



// void PR205_A1::s_valve_opening(std::size_t idx)
// {
//     eng::log::info("{}: {}({})", name(), __func__, idx);
//
//     // Еще не открыта
//     if (!bs_0х4005_.test(idx) && modbus_unit::is_online())
//         return;
//
//     auto &valve = valves_[idx];
//
//     if (!bs_0х4005_.test(idx))
//     {
//         valve.state = &PR205_A1::s_valve_closed;
//         node::set_ready(valve.ictl);
//         return;
//     }
//
//     valve.state = &PR205_A1::s_valve_opened;
// }

// void PR205_A1::s_valve_opened(std::size_t idx)
// {
//     eng::log::info("{}: {}({})", name(), __func__, idx);
//
//     auto &valve = valves_[idx];
//
//     // Все еще открыта
//     if (bs_0х4005_.test(idx) && node::is_active(valve.ictl))
//     {
//         eng::log::info("\tSTAY OPENED");
//         return;
//     }
//
//     if (!modbus_unit::is_online())
//     {
//         eng::log::info("\tSTAY OPENED BY OFFLINE");
//         return;
//     }
//
//     if (!bs_0х4005_.test(idx))
//     {
//         eng::log::info("\tSWITCH TO CLOSED");
//         valve.state = &PR205_A1::s_valve_closed;
//         return;
//     }
//
//     eng::log::info("\tSEND COMMAND TO CLOSE");
//
//     // Отправляем команду закрытия
//     auto bitset = bs_0х4005_;
//     bitset.set(idx, false);
//     modbus_unit::write_single(0x4005, bitset.to_ulong());
//     valve.state = &PR205_A1::s_valve_closing;
// }

// void PR205_A1::s_valve_closing(std::size_t idx)
// {
//     eng::log::info("{}: {}({})", name(), __func__, idx);
//
//     // Еще не закрыта
//     if (bs_0х4005_.test(idx) && modbus_unit::is_online())
//     {
//         eng::log::info("\tWAIT CLOSING");
//         return;
//     }
//
//     auto &valve = valves_[idx];
//
//     if (bs_0х4005_.test(idx))
//     {
//         eng::log::info("\tSWITCH TO OPENED");
//         valve.state = &PR205_A1::s_valve_opened;
//         return;
//     }
//
//     eng::log::info("\tSWITCH TO CLOSED");
//
//     // Закрыта
//     valve.state = &PR205_A1::s_valve_closed;
// }

// void PR205_A1::s_valve_closed(std::size_t idx)
// {
//     eng::log::info("{}: {}({})", name(), __func__, idx);
//
//     auto &valve = valves_[idx];
//
//     // Все еще закрыта
//     if (!bs_0х4005_.test(idx) && !node::is_active(valve.ictl))
//     {
//         eng::log::info("\tSTAY CLOSED");
//         return;
//     }
//
//     if (!modbus_unit::is_online())
//     {
//         eng::log::info("\tSTAY CLOSED BY OFFLINE");
//         return;
//     }
//
//     if (bs_0х4005_.test(idx))
//     {
//         eng::log::info("\tSWITCH TO OPENED");
//         valve.state = &PR205_A1::s_valve_opened;
//         return;
//     }
//
//     eng::log::info("\tSEND COMMAND TO OPEN");
//
//     // Отправляем команду открытия
//     auto bitset = bs_0х4005_;
//     bitset.set(idx, true);
//     modbus_unit::write_single(0x4005, bitset.to_ulong());
//     valve.state = &PR205_A1::s_valve_opening;
// }


