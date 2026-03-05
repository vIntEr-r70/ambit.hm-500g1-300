#include "PR205-A2.hpp"

namespace devices
{

    PR205_A2::PR205_A2()
        : modbus_device("10.0.0.12")
    {
    }

    void PR205_A2::fc_set(std::size_t idx, std::int16_t value)
    {
        set(0x4002 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_A2::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x400A + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_A2::dp_set(std::size_t idx, std::int16_t value)
    {
        set(0x4011 + idx, static_cast<std::uint16_t>(value));
    }

}
