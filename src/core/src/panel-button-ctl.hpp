#pragma once

#include <eng/sibus/node.hpp>

class panel_button_ctl final
    : public eng::sibus::node
{
    eng::sibus::output_wire_id_t ctl_;

public:

    panel_button_ctl(std::string_view);

private:

    void turn_on_action();

    void turn_off_action();
};
