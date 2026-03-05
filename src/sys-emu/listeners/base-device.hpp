#pragma once

#include <string_view>
#include <functional>
#include <cstdint>
#include <string>

namespace devices
{

    using bitset_callback = std::function<void(bool)>;
    using regset_callback = std::function<void(std::uint16_t)>;

    class base_device
    {
        std::string name_;
        bool online_{ true };

    protected:

        base_device(std::string_view);

        virtual ~base_device() = default;

    public:

        void set_online(bool);

        bool online() const noexcept { return online_; }

        std::string const &name() const noexcept { return name_; }
    };

    class modbus_device
        : public base_device
    {
        std::unordered_map<std::uint16_t, std::uint16_t> map_;

    protected:

        modbus_device(std::string_view);

    public:

        std::uint16_t get(std::uint16_t);

        void set(std::uint16_t, std::uint16_t);

    protected:

        void set(std::uint16_t, std::size_t, bool);

    private:

        virtual void changed(std::uint16_t, std::uint16_t) { }
    };

}
