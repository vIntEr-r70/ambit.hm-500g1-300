#include "PR205-B6.hpp"

namespace devices
{

    PR205_B6::PR205_B6()
        : modbus_device("10.0.0.14")
    {
    }

    void PR205_B6::fc_set(std::size_t idx, std::int16_t value)
    {
        set(0x4006 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_B6::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x4002 + idx, static_cast<std::uint16_t>(value));
    }

}
