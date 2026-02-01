#pragma once

#include "unit-conf-loader.hpp"

class modbus_unit;

namespace units_interaction
{

    std::size_t add_new_unit(modbus_unit *);

    void write_single(std::size_t, std::uint16_t, std::uint16_t);

    void add_last_unit_record(std::size_t, std::size_t, unit_conf_loader::record_t const &);

    void start();

}

