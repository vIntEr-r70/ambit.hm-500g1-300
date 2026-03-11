#include "FC.hpp"

#include <bitset>

namespace devices
{

    FC::FC()
        : modbus_device("ttyS0.10")
    {
    }

    void FC::changed(std::uint16_t address, std::uint16_t value)
    {
        switch(address)
        {
        case 0xA410: {
            std::bitset<2> bits(value);
            if (bits.test(1)) reset_callback_();
            power_callback_(bits.test(0));
            break; }
        case 0xA420:
            iset_callback_(value * 0.1);
            break;
        case 0xA421:
            uset_callback_(value);
            break;
        case 0xA422:
            pset_callback_(value * 0.1);
            break;
        }
    }

    void FC::on_ui_set(regset_callback callback)
    {
        ui_set_callback_ = std::move(callback);
    }

}
