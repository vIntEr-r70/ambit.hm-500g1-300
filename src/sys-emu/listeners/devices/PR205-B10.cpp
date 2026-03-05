#include "PR205-B10.hpp"

namespace devices
{

    PR205_B10::PR205_B10()
        : modbus_device("10.0.0.15")
    {
    }

    void PR205_B10::fc_set(std::size_t idx, std::int16_t value)
    {
        set(0x4006 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_B10::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x4002 + idx, static_cast<std::uint16_t>(value));
    }
}
