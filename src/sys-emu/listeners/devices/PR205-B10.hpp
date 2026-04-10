#pragma once

#include "../base-device.hpp"

namespace devices
{

    class PR205_B10 final
        : public modbus_device
    {
        std::unordered_map<std::size_t, bitset_callback> callbacks_;

    public:

        PR205_B10();

    public:

        void fc_set(std::size_t, std::int16_t);

        void dt_set(std::size_t, std::int16_t);

        bool bit_get(std::size_t);

        void bit_set(std::size_t, bool);

        void bitset(std::size_t, bitset_callback);

        bool bit_p_get(std::size_t);

        void bit_p_set(std::size_t, bool);

    private:

        void changed(std::uint16_t, std::uint16_t) override final;
    };

}


