#include "syslink.hpp"

#include <unordered_map>

namespace syslink
{

    static std::unordered_map<std::size_t, devices::base_device *> devices;

    namespace details
    {

        void add_new_device(std::size_t id, devices::base_device *device)
        {
            devices[id] = device;
        }

        devices::base_device *find_device_by_type_id(std::size_t id)
        {
            return devices[id];
        }

    }

}

