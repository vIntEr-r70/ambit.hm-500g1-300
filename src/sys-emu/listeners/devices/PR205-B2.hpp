#pragma once

#include "../base-device.hpp"

namespace devices
{

    class PR205_B2 final
        : public modbus_device
    {
    public:

        PR205_B2();

    public:

        void fc_set(std::size_t, std::int16_t);

        void dt_set(std::size_t, std::int16_t);

    private:

        void changed(std::uint16_t, std::uint16_t) override final;
    };

}


