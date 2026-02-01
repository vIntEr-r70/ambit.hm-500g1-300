#pragma once

#include <cstdint>
#include <span>
#include <string_view>

class modbus_unit;

namespace modbus_rtu_master
{

    void add_new_unit(modbus_unit *);

    void restart(modbus_unit *);

    void init(std::string_view);

    void destroy();

    void write_single(modbus_unit const *, std::uint16_t, std::uint16_t);

    void write_multiple(modbus_unit const *, std::uint16_t, std::span<std::uint16_t const>);

}

