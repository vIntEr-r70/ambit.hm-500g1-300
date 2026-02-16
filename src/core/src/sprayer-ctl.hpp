#pragma once

#include <eng/sibus/node.hpp>

class sprayer_ctl final
    : public eng::sibus::node
{
    eng::sibus::input_wire_id_t ictl_;
    eng::sibus::output_port_id_t on_off_;

    bool linked_{ false };
    bool powered_{ false };

public:

    sprayer_ctl(std::string_view);

private:

    void activate();

    void deactivate();

private:

    void update_real_state(bool, bool);
};
