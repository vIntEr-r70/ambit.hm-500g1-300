#include "units/PLC110.hpp"

#include <eng/log.hpp>

#include <algorithm>

PLC110::PLC110(std::uint8_t slave_id)
    : eng::sibus::node("PLC110")
    , eng::modbus::unit(slave_id)
{
    std::size_t idx = unit::add_read_task(0x0000, 2, 100);
    read_task_handlers_[idx] = &PLC110::read_spin_done;
    spin_.port_id = node::add_output_port("spin");

    idx = unit::add_read_task(0x0016, 1, 100);
    read_task_handlers_[idx] = &PLC110::read_mk_1_done;
    for (std::size_t i = 0; i < mk_[0].size(); ++i)
        mk_[0][i].port_id = node::add_output_port(std::format("m1b{}", i + 1));

    idx = unit::add_read_task(0x0019, 1, 100);
    read_task_handlers_[idx] = &PLC110::read_mk_2_done;
    for (std::size_t i = 0; i < mk_[1].size(); ++i)
        mk_[1][i].port_id = node::add_output_port(std::format("m2b{}", i + 1));

    idx = unit::add_read_task(0x0004, 2, 100);
    read_task_handlers_[idx] = &PLC110::read_plc_done;
    for (std::size_t i = 0; i < plc_.size(); ++i)
        plc_[i].port_id = node::add_output_port(std::format("b{}", i + 5));

    for (std::size_t i = 0; i < outputs_.size(); ++i)
    {
        node::add_input_port(std::format("b{}", i + 5), [this, i](eng::abc::pack args)
        {
            outputs_.set(i, eng::abc::get<bool>(args));
            write_outputs();
        });
    }

    for (std::size_t imk = 0; imk < mk_outputs_.size(); ++imk)
    {
        auto &outputs = mk_outputs_[imk];
        for (std::size_t i = 0; i < outputs.size(); ++i)
        {
            node::add_input_port(std::format("m{}b{}", imk + 1, i + 1), [this, imk, i](eng::abc::pack args)
            {
                mk_outputs_[imk].set(i, eng::abc::get<bool>(args));
                write_mk_outputs(imk);
            });
        }
    }
}

void PLC110::read_task_done(std::size_t idx, readed_regs_t regs)
{
    (this->*read_task_handlers_[idx])(regs);
}

void PLC110::connection_was_lost()
{
    std::ranges::for_each(plc_, [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });

    std::ranges::for_each(mk_[0], [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });

    std::ranges::for_each(mk_[1], [this](auto &item)
    {
        item.initialized = false;
        node::set_port_value(item.port_id, { });
    });
}

void PLC110::now_unit_online()
{
    update_outputs();

    for (std::size_t i = 0; i < mk_outputs_.size(); ++i)
        update_mk_outputs(i);
}

void PLC110::read_spin_done(readed_regs_t regs)
{
    std::int32_t value;
    std::memcpy(&value, regs.data(), sizeof(value));

    if (spin_.value == value && spin_.initialized)
        return;

    spin_.value = value;
    spin_.initialized = true;

    node::set_port_value(spin_.port_id, { spin_.value });
}

void PLC110::read_plc_done(readed_regs_t regs)
{
    std::uint32_t value;
    std::memcpy(&value, regs.data(), sizeof(value));

    std::bitset<32> bitset(value);

    for (std::size_t i = 0; i < plc_.size(); ++i)
    {
        auto &item = plc_[i];

        if (item.value == bitset[i] && item.initialized)
            continue;

        item.value = bitset[i];
        item.initialized = true;

        node::set_port_value(item.port_id, { item.value });

        eng::log::info("{}[b{}]: {}", name(), i + 5, item.value ? "ON" : "OFF");
    }
}

void PLC110::read_mk_1_done(readed_regs_t regs)
{
    read_mk_done(0, regs);
}

void PLC110::read_mk_2_done(readed_regs_t regs)
{
    read_mk_done(1, regs);
}

void PLC110::read_mk_done(std::size_t idx, readed_regs_t regs)
{
    auto &mk = mk_[idx];

    std::bitset<8> bitset(regs[0]);

    for (std::size_t i = 0; i < mk.size(); ++i)
    {
        auto &item = mk[i];

        if (item.value == bitset[i] && item.initialized)
            continue;

        item.value = bitset[i];
        item.initialized = true;

        node::set_port_value(item.port_id, { item.value });

        eng::log::info("{}[m{}b{}]: {}", name(), idx + 1, i + 1, item.value ? "ON" : "OFF");
    }
}

void PLC110::write_outputs()
{
    if (!unit::is_online())
        return;
    update_outputs();
}

void PLC110::update_outputs()
{
    std::uint16_t values[2];
    values[0] = outputs_.to_ulong() & 0xFFFF;
    values[1] = (outputs_.to_ulong() >> 16) & 0x000F;

    unit::write_multiple(0x0006, std::span{ values });

    eng::log::info("PLC110::OUTPUTS: {}", outputs_.to_string());
}

void PLC110::write_mk_outputs(std::size_t imk)
{
    if (!unit::is_online())
        return;
    update_mk_outputs(imk);
}

void PLC110::update_mk_outputs(std::size_t imk)
{
    static std::uint16_t address[] { 0x0015, 0x0018 };

    std::uint16_t value = mk_outputs_[imk].to_ulong() & 0x00FF;
    unit::write_single(address[imk], value);

    eng::log::info("PLC110::MK[{}]::OUTPUTS: {}", imk, mk_outputs_[imk].to_string());
}
