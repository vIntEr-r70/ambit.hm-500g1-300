#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "value-holder.hpp"

class ethercat_slave;

namespace ethercat
{

    // struct slave_info_t
    // {
    //     std::uint16_t alias;
    //     std::uint16_t position;
    //     std::uint32_t VendorID;
    //     std::uint32_t ProductCode;
    // };

    // struct id_t
    // {
    //     explicit id_t(std::size_t idx) : idx{ idx } { }
    //     operator std::size_t() const noexcept { return idx; }
    //     std::size_t idx;
    // };

    // id_t register_slave(slave_info_t);

    namespace pdo
    {

        void add(ethercat_slave*, std::uint16_t, std::uint8_t, value_holder_base_ro &);

        void add(ethercat_slave*, std::uint16_t, std::uint8_t, value_holder_base_rw &);

    }

    // bool init(std::function<void(std::string const &, std::string const &, std::string const &)>);
    bool init();

    // slave_id_t add_slave(std::string const &, slave_config_t);

    void add_pdo_transmit(std::uint16_t, std::uint8_t, std::string_view);

    void add_pdo_receive(std::uint16_t, std::uint8_t, std::string_view);

    void add_ro_sdo(std::uint16_t, std::uint8_t, std::string_view);

    void add_rw_sdo(std::uint16_t, std::uint8_t, std::string_view);

    void set_bit(std::string_view, std::string_view, std::string_view);

    void reset_bit(std::string_view, std::string_view, std::string_view);

    void set_value(std::string_view, std::string_view, std::string_view);
};
