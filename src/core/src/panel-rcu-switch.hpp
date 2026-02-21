#pragma once

#include <eng/sibus/node.hpp>

class panel_rcu_switch final
    : public eng::sibus::node
{
    eng::sibus::output_port_id_t panel_;
    eng::sibus::output_port_id_t rcu_;

    eng::sibus::output_port_id_t mode_;

    eng::sibus::output_port_id_t rcu_led_;

public:

    panel_rcu_switch();

private:

    void update_output(eng::abc::pack);
};
