#pragma once

#include <eng/sibus/node.hpp>

class panel_switch_2_ctl final
    : public eng::sibus::node
{
    std::array<eng::sibus::output_wire_id_t, 2> ctl_;

public:

    panel_switch_2_ctl(std::string_view);

private:

    void activate(std::size_t);

    void deactivate(std::size_t);
};
