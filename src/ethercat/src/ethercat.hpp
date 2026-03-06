#pragma once

#include <cstdint>
#include <string_view>

#include "value-holder.hpp"

class ethercat_slave;

namespace ethercat
{

    namespace pdo
    {

        void add(ethercat_slave*, std::uint16_t, std::uint8_t, value_holder_base_ro &);

        void add(ethercat_slave*, std::uint16_t, std::uint8_t, value_holder_base_rw &);

    }

    bool init();

    void add_pdo_transmit(std::uint16_t, std::uint8_t, std::string_view);

    void add_pdo_receive(std::uint16_t, std::uint8_t, std::string_view);

    void add_ro_sdo(std::uint16_t, std::uint8_t, std::string_view);

    void add_rw_sdo(std::uint16_t, std::uint8_t, std::string_view);

    void set_bit(std::string_view, std::string_view, std::string_view);

    void reset_bit(std::string_view, std::string_view, std::string_view);

    void set_value(std::string_view, std::string_view, std::string_view);
};
