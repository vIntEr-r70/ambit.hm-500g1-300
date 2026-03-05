#pragma once

#include "../base-device.hpp"

namespace devices
{

    class FC final
        : public modbus_device
    {
        regset_callback ui_set_callback_;

    public:

        std::function<void(double)> iset_callback_;
        std::function<void(double)> uset_callback_;
        std::function<void(double)> pset_callback_;

        std::function<void()> reset_callback_;
        std::function<void(bool)> power_callback_;

    public:

        FC();

    public:

        void on_ui_set(regset_callback);

    private:

        void changed(std::uint16_t, std::uint16_t) override final;
    };

}

