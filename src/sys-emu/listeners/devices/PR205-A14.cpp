#include "PR205-A14.hpp"

namespace devices
{

    PR205_A14::PR205_A14()
        : modbus_device("10.0.0.16")
    {
    }

    void PR205_A14::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x4002 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_A14::dp_set(std::size_t idx, std::int16_t value)
    {
        set(0x4004 + idx, static_cast<std::uint16_t>(value));
    }

}
