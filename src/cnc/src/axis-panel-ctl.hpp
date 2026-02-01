#pragma once

#include <eng/sibus/node.hpp>

class axis_panel_ctl final
    : public eng::sibus::node
{
    char axis_;
    double speed_;

    eng::sibus::output_wire_id_t ctl_;

public:

    axis_panel_ctl(char);

private:

    void spin_command(double);
};
