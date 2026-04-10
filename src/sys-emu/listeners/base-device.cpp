#include "base-device.hpp"
#include "modbus-tcp-server.hpp"

#include <eng/log.hpp>

namespace devices
{

    base_device::base_device(std::string_view name)
        : name_(name)
    { }

    void base_device::set_online(bool value)
    {
        online_ = value;
    }

    void base_device::stop()
    {
    }


    modbus_device::modbus_device(std::string_view name)
        : base_device(name)
    {
        listener_id_ = mts::add_listener(*this);
    }

    modbus_device::~modbus_device()
    {
        if (listener_id_)
            mts::remove_listener(listener_id_);
    }

    void modbus_device::stop()
    {
        if (listener_id_)
            mts::remove_listener(listener_id_);
        listener_id_.reset();
    }

    void modbus_device::set(std::uint16_t address, std::size_t idx, bool value)
    {
        auto &mask = map_[address];
        if (value) mask |= (1 << idx);
        else mask &= ~(1 << idx);
    }

    std::uint16_t modbus_device::get(std::uint16_t address)
    {
        return map_[address];
    }

    void modbus_device::set(std::uint16_t address, std::uint16_t value)
    {
        auto &current_value = map_[address];
        if (current_value == value)
            return;
        eng::log::info("SET[{}]: {:04X} = {:04X}", name(), address, value);
        current_value = value;
        changed(address, value);
    }

}

