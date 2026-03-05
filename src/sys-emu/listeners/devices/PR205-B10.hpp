#pragma once

#include "../base-device.hpp"

namespace devices
{

    class PR205_B10 final
        : public modbus_device
    {
    public:

        PR205_B10();

    public:

        void fc_set(std::size_t, std::int16_t);

        void dt_set(std::size_t, std::int16_t);
    };

}


