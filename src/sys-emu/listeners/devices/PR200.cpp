#include "PR200.hpp"

#include <bitset>
#include <algorithm>

namespace devices
{

    PR200::PR200()
        : modbus_device("ttyS0.11")
    { }

    void PR200::fc_set(std::size_t idx, std::int16_t value)
    {
        set(0x0200 + idx, static_cast<std::uint16_t>(value));
    }

    void PR200::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x0202 + idx, static_cast<std::uint16_t>(value));
    }

    bool PR200::bit_get(std::size_t idx)
    {
        std::bitset<8> bitset(get(0x021E));
        return bitset.test(idx);
    }

    void PR200::bit_set(std::size_t idx, bool value)
    {
        std::bitset<8> bitset(get(0x021E));
        bitset.set(idx, value);
        set(0x021E, bitset.to_ulong());
    }

    void PR200::bitset(std::size_t idx, bitset_callback callback)
    {
        callbacks_[idx] = std::move(callback);
    }

    void PR200::changed(std::uint16_t address, std::uint16_t value)
    {
        switch (address)
        {
        case 0x021E:
            std::ranges::for_each(callbacks_, [&](auto const &pair)
            {
                std::bitset<8> bitset(value);
                (pair.second)(bitset.test(pair.first));
            });
            break;
        }
    }

}
