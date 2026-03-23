#include "PR205-A1.hpp"

#include <bitset>
#include <algorithm>

namespace devices
{

    PR205_A1::PR205_A1()
        : modbus_device("10.0.0.11")
    {
    }

    void PR205_A1::fc_set(std::size_t idx, std::int16_t value)
    {
        set(0x4001 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_A1::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x4006 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_A1::dp_set(std::size_t idx, std::int16_t value)
    {
        set(0x4008 + idx, static_cast<std::uint16_t>(value));
    }

    bool PR205_A1::bit_get(std::size_t idx)
    {
        std::bitset<8> bitset(get(0x4005));
        return bitset.test(idx);
    }

    void PR205_A1::bit_set(std::size_t idx, bool value)
    {
        std::bitset<8> bitset(get(0x4005));
        bitset.set(idx, value);
        set(0x4005, bitset.to_ulong());
    }

    void PR205_A1::bitset(std::size_t idx, bitset_callback callback)
    {
        callbacks_[idx] = std::move(callback);
    }

    void PR205_A1::changed(std::uint16_t address, std::uint16_t value)
    {
        switch (address)
        {
        case 0x4005:
            std::ranges::for_each(callbacks_, [&](auto const &pair)
            {
                std::bitset<8> bitset(value);
                (pair.second)(bitset.test(pair.first));
            });
            break;
        }
    }

}

