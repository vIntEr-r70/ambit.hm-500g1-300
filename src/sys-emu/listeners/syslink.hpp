#pragma once

#include "base-device.hpp"

namespace syslink
{

    namespace details
    {

        class type_tracker
        {
            inline static std::size_t next_id_{ 0 };

        public:

            template <typename T>
            static int get_id()
            {
                static std::size_t const id{ next_id_++ };
                return id;
            }
        };

        devices::base_device *find_device_by_type_id(std::size_t);

        void add_new_device(std::size_t, devices::base_device *);

    }

    template <typename T>
    void add_new_device()
    {
        devices::base_device *base = new T();
        details::add_new_device(details::type_tracker::get_id<T>(), base);
    }

    template <typename T>
    T& device()
    {
        devices::base_device *base = details::find_device_by_type_id(details::type_tracker::get_id<T>());
        return *static_cast<T*>(base);
    }

    void destroy();

}

