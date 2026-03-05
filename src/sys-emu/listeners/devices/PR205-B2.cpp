#include "PR205-B2.hpp"

namespace devices
{

    PR205_B2::PR205_B2()
        : modbus_device("10.0.0.13")
    {
    }

    void PR205_B2::fc_set(std::size_t idx, std::int16_t value)
    {
        set(0x4006 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_B2::dt_set(std::size_t idx, std::int16_t value)
    {
        set(0x4002 + idx, static_cast<std::uint16_t>(value));
    }

    void PR205_B2::changed(std::uint16_t address, std::uint16_t value)
    {
        switch(address)
        {
        case 0x4001:
            break;
        }
    }

}
