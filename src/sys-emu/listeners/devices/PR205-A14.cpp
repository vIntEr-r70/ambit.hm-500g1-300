#include "PR205-A14.hpp"

#include <bitset>
#include <algorithm>

namespace devices
{

    PR205_A14::PR205_A14()
        : modbus_device("10.0.0.16")
    { }

    void PR205_A14::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x4002 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_A14::dp_set(std::size_t idx, std::int16_t value)
    {
        set(0x4004 + idx, static_cast<std::uint16_t>(value));
    }

    bool PR205_A14::bit_get(std::size_t idx)
    {
        std::bitset<8> bitset(get(0x4001));
        return bitset.test(idx);
    }

    void PR205_A14::bit_set(std::size_t idx, bool value)
    {
        std::bitset<8> bitset(get(0x4001));
        bitset.set(idx, value);
        set(0x4001, bitset.to_ulong());
    }

    bool PR205_A14::bit_va_get(std::size_t idx)
    {
        std::bitset<8> bitset(get(0x400A));
        return bitset.test(idx);
    }

    void PR205_A14::bit_va_set(std::size_t idx, bool value)
    {
        std::bitset<8> bitset(get(0x400A));
        bitset.set(idx, value);
        set(0x400A, bitset.to_ulong());
    }

    bool PR205_A14::bit_p_get(std::size_t idx)
    {
        std::bitset<8> bitset(get(0x4000));
        return bitset.test(idx);
    }

    void PR205_A14::bit_p_set(std::size_t idx, bool value)
    {
        std::bitset<8> bitset(get(0x4000));
        bitset.set(idx, value);
        set(0x4000, bitset.to_ulong());
    }

    void PR205_A14::bitset(std::size_t idx, bitset_callback callback)
    {
        callbacks_[idx] = std::move(callback);
    }

    void PR205_A14::bitset_va(std::size_t idx, bitset_callback callback)
    {
        va_callbacks_[idx] = std::move(callback);
    }

    void PR205_A14::changed(std::uint16_t address, std::uint16_t value)
    {
        switch (address)
        {
        case 0x4001:
            std::ranges::for_each(callbacks_, [&](auto const &pair)
            {
                std::bitset<8> bitset(value);
                (pair.second)(bitset.test(pair.first));
            });
            break;
        }
    }

}
