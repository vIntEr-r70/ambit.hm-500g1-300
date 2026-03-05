#pragma once

#include "../base-device.hpp"

namespace devices
{

    class PR205_A14 final
        : public modbus_device
    {
    public:

        PR205_A14();

    public:

        void dt_set(std::size_t, std::int16_t);

        void dp_set(std::size_t, std::int16_t);
    };

}


