#include "PLC110.hpp"

#include <algorithm>
#include <bitset>

namespace devices
{

    PLC110::PLC110()
        : modbus_device("10.0.0.10")
    {
        plc_bitset(22, true);
        plc_bitset(24, true);
        plc_bitset(26, true);
        plc_bitset(29, true);
        plc_bitset(31, true);
        plc_bitset(35, true);

        m2_bitset(2, true);
    }

    void PLC110::plc_bitset(std::size_t idx, bool value)
    {
        idx -= 5;
        set(0x0004 + (idx / 16), idx % 16, value);
    }

    void PLC110::m1_bitset(std::size_t idx, bool value)
    {
        idx -= 1;
        set(0x0016, idx, value);
    }

    void PLC110::m2_bitset(std::size_t idx, bool value)
    {
        idx -= 1;
        set(0x0019, idx, value);
    }

    void PLC110::plc_bitset(std::size_t idx, bitset_callback callback)
    {
        idx -= 5;
        plc_bitset_callbacks_[idx / 16][idx % 16] = std::move(callback);
    }

    void PLC110::m1_bitset(std::size_t idx, bitset_callback callback)
    {
        idx -= 1;
        m1_bitset_callbacks_[idx] = std::move(callback);
    }

    void PLC110::m2_bitset(std::size_t idx, bitset_callback callback)
    {
        idx -= 1;
        m2_bitset_callbacks_[idx] = std::move(callback);
    }

    void PLC110::changed(std::uint16_t address, std::uint16_t value)
    {
        switch(address)
        {
        case 0x0006:
        case 0x0007:
            std::ranges::for_each(plc_bitset_callbacks_[address - 0x0006], [&](auto const &pair)
            {
                std::bitset<16> bitset(value);
                (pair.second)(bitset.test(pair.first));
            });
            break;
        case 0x0015:
            std::ranges::for_each(m1_bitset_callbacks_, [&](auto const &pair)
            {
                std::bitset<16> bitset(value);
                (pair.second)(bitset.test(pair.first));
            });
            break;
        case 0x0018:
            std::ranges::for_each(m2_bitset_callbacks_, [&](auto const &pair)
            {
                std::bitset<16> bitset(value);
                (pair.second)(bitset.test(pair.first));
            });
            break;
        }
    }

}
