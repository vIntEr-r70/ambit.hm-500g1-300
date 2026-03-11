#include "PR200.hpp"

namespace devices
{

    PR200::PR200()
        : modbus_device("ttyS0.11")
    {
    }

    void PR200::fc_set(std::size_t idx, std::int16_t value)
    {
        set(0x0200 + idx, static_cast<std::uint16_t>(value));
    }

    void PR200::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x0202 + idx, static_cast<std::uint16_t>(value));
    }

}
