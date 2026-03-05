#pragma once

#include "../base-device.hpp"

namespace devices
{

    class PR200 final
        : public modbus_device
    {
    public:

        PR200();

    public:

        void fc_set(std::size_t, std::int16_t);

        void dt_set(std::size_t, std::int16_t);
    };

}


