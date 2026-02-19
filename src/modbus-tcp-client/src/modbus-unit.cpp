#include "modbus-unit.hpp"
#include "units-interaction.hpp"

#include <eng/log.hpp>

#include <cstring>

modbus_unit::modbus_unit(std::string_view host, std::uint16_t port, std::uint8_t id)
    : host_(host)
    , port_(port)
    , id_(id)
    , idx_(units_interaction::add_new_unit(this))
{ }

std::size_t modbus_unit::add_read_task(std::uint16_t address, std::size_t number_of, std::uint32_t msecs)
{
    std::size_t idx = read_holding_.size();
    read_holding_.emplace_back(address, number_of, msecs);
    return idx;
}

void modbus_unit::read_holding_done(std::size_t idx, std::span<std::uint8_t const> readed)
{
    result_.clear();

    std::uint8_t const *ptr = readed.data();
    auto const end = ptr + readed.size();

    std::array<std::uint8_t, 2> pair;
    while ((ptr + pair.size()) <= end)
    {
        pair = { *ptr, *(ptr + 1) };
        std::swap(pair[0], pair[1]);

        result_.emplace_back();
        std::memcpy(&result_.back(), pair.data(), pair.size());

        ptr += pair.size();
    }

    read_task_done(idx, result_);
}

void modbus_unit::write_done()
{
    write_task_done(0);
}

void modbus_unit::write_single(std::uint16_t address, std::uint16_t value)
{
    units_interaction::write_single(idx_, address, value);
}

void modbus_unit::write_multiple(std::uint16_t address, std::span<std::uint16_t const> values)
{
    units_interaction::write_multiple(idx_, address, values);
}

void modbus_unit::connected()
{
    if (online_ && *online_)
        return;
    online_ = true;
    now_unit_online();
}

void modbus_unit::disconnected()
{
    if (online_ && !*online_)
        return;
    online_ = false;
    connection_was_lost();
}

