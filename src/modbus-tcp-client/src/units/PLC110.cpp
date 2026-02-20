#include "units/PLC110.hpp"

#include <eng/log.hpp>

PLC110::PLC110(std::string_view host, std::uint16_t port)
    : eng::sibus::node("PLC110")
    , modbus_unit(host, port, 100)
{
    std::size_t idx;

    idx = modbus_unit::add_read_task(0x0000, 2, 100);
    read_task_handlers_[idx] = &PLC110::read_spin_done;
    spin_.port_id = node::add_output_port("spin");

    idx = modbus_unit::add_read_task(0x0016, 1, 100);
    read_task_handlers_[idx] = &PLC110::read_mk_1_done;
    for (std::size_t i = 0; i < mk_bits_[0].bitset.size(); ++i)
        mk_bits_[0].ports_id[i] = node::add_output_port(std::format("m1b{}", i + 1));

    idx = modbus_unit::add_read_task(0x0019, 1, 100);
    read_task_handlers_[idx] = &PLC110::read_mk_2_done;
    for (std::size_t i = 0; i < mk_bits_[1].bitset.size(); ++i)
        mk_bits_[1].ports_id[i] = node::add_output_port(std::format("m2b{}", i + 1));

    idx = modbus_unit::add_read_task(0x0004, 2, 100);
    read_task_handlers_[idx] = &PLC110::read_bits_done;
    for (std::size_t i = 0; i < bits_.bitset.size(); ++i)
        bits_.ports_id[i] = node::add_output_port(std::format("b{}", i + 5));

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

void PLC110::read_bits_done(readed_regs_t regs)
{
    std::uint32_t value;
    std::memcpy(&value, regs.data(), sizeof(value));

    decltype(bits_.bitset) bitset(value);
    decltype(bits_.bitset) diff_bits = bitset ^ bits_.bitset;

    if (!diff_bits.count() && bits_.initialized)
        return;

    bits_.bitset = bitset;

    for (std::size_t i = 0; i < bits_.bitset.size(); ++i)
    {
        if (!diff_bits[i] && bits_.initialized)
            continue;
        eng::log::info("PLC110[b{}]: {}", i + 5, bitset[i] ? "ON" : "OFF");
        node::set_port_value(bits_.ports_id[i], { bits_.bitset.test(i) });
    }

    bits_.initialized = true;
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
    auto &bits = mk_bits_[idx];

    decltype(bits.bitset) bitset(regs[0]);
    decltype(bits.bitset) diff_bits = bitset ^ bits.bitset;

    if (!diff_bits.count() && bits.initialized)
        return;

    bits.bitset = bitset;

    for (std::size_t i = 0; i < bits.bitset.size(); ++i)
    {
        if (!diff_bits[i] && bits.initialized)
            continue;
        eng::log::info("PLC110[m{}b{}]: {}", idx + 1, i + 1, bitset[i] ? "ON" : "OFF");
        node::set_port_value(bits.ports_id[i], { bits.bitset.test(i) });
    }

    bits.initialized = true;
}

void PLC110::write_outputs()
{
    if (!modbus_unit::is_online())
        return;
    update_outputs();
}

void PLC110::update_outputs()
{
    std::uint16_t values[2];
    values[0] = outputs_.to_ulong() & 0xFFFF;
    values[1] = (outputs_.to_ulong() >> 16) & 0x000F;

    modbus_unit::write_multiple(0x0006, std::span{ values });

    eng::log::info("PLC110::OUTPUTS: {}", outputs_.to_string());
}

void PLC110::write_mk_outputs(std::size_t imk)
{
    if (!modbus_unit::is_online())
        return;
    update_mk_outputs(imk);
}

void PLC110::update_mk_outputs(std::size_t imk)
{
    static std::uint16_t address[] { 0x0015, 0x0018 };

    std::uint16_t value = mk_outputs_[imk].to_ulong() & 0x00FF;
    modbus_unit::write_single(address[imk], value);

    eng::log::info("PLC110::MK[{}]::OUTPUTS: {}", imk, mk_outputs_[imk].to_string());
}
