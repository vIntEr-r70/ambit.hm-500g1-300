#pragma once

#include <eng/pipe/listener.hpp>

namespace devices {
    class modbus_device;
}

namespace mts
{

    eng::pipe::listener_id_t add_listener(devices::modbus_device &);

    void remove_listener(eng::pipe::listener_id_t);

}

