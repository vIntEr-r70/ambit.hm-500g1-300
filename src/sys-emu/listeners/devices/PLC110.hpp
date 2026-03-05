#pragma once

#include "../base-device.hpp"

#include <cstddef>

namespace devices
{

    class PLC110 final
        : public modbus_device
    {
        std::unordered_map<std::size_t, bitset_callback> plc_bitset_callbacks_[2];
        std::unordered_map<std::size_t, bitset_callback> m1_bitset_callbacks_;
        std::unordered_map<std::size_t, bitset_callback> m2_bitset_callbacks_;

    public:

        PLC110();

    public:

        void plc_bitset(std::size_t, bool);

        void m1_bitset(std::size_t, bool);

        void m2_bitset(std::size_t, bool);

        void plc_bitset(std::size_t, bitset_callback);

        void m1_bitset(std::size_t, bitset_callback);

        void m2_bitset(std::size_t, bitset_callback);

    private:

        void changed(std::uint16_t, std::uint16_t) override final;
    };

}
