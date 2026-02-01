#include "modbus-unit.hpp"
#include "modbus-rtu-master.hpp"

#include <eng/timer.hpp>

#include <cstring>

modbus_unit::modbus_unit(std::uint8_t id)
    : id_(id)
{
    modbus_rtu_master::add_new_unit(this);

    // Таймер, работающий с целю восстановления связи с устройством
    offline_timer_id_ = eng::timer::create([this]
    {
        modbus_rtu_master::restart(this);
    });
}

std::size_t modbus_unit::add_read_task(std::uint16_t address, std::size_t number_of, std::uint32_t msecs)
{
    std::size_t idx = read_holding_.size();
    read_holding_.emplace_back(address, number_of, msecs);
    return idx;
}

std::size_t modbus_unit::write_single(std::uint16_t address, std::uint16_t value)
{
    modbus_rtu_master::write_single(this, address, value);
    return 0;
}

std::size_t modbus_unit::write_multiple(std::uint16_t address, std::span<std::uint16_t const> data)
{
    modbus_rtu_master::write_multiple(this, address, data);
    return 0;
}

void modbus_unit::offline()
{
    eng::timer::start(offline_timer_id_, std::chrono::seconds(1));

    online(false);
}

void modbus_unit::read_holding_done(std::size_t idx, std::span<std::uint8_t const> readed)
{
    eng::timer::stop(offline_timer_id_);

    online(true);

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

void modbus_unit::write_done(std::size_t)
{
}

void modbus_unit::online(bool value)
{
    if (online_ && online_.value() == value)
        return;
    online_ = value;

    if (value)
        now_unit_online();
    else
        connection_was_lost();
}

