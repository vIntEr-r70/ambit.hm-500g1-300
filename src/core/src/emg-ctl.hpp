#pragma once

#include <eng/sibus/node.hpp>

class emg_ctl final
    : public eng::sibus::node
{
    eng::sibus::output_port_id_t emg_;

public:

    emg_ctl();
};
