#include "PR205-A1.hpp"

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

}
