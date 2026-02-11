#pragma once

#include <eng/sibus/node.hpp>

class drainage_ctl final
    : public eng::sibus::node
{
    eng::sibus::output_wire_id_t va_ctl_;

    eng::sibus::output_port_id_t pump_out_;

    std::optional<bool> pmin_;
    std::optional<bool> pmax_;

    void (drainage_ctl::*pump_)();

public:

    drainage_ctl();

private:

    void turned_off();

    void turned_on();
};

