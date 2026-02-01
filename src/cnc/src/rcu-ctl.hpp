#pragma once

#include <eng/sibus/node.hpp>

class rcu_ctl final
    : public eng::sibus::node
{
    char axis_{ '\0' };

    eng::sibus::input_wire_id_t iwire_;
    std::unordered_map<char, eng::sibus::output_wire_id_t> owires_;

public:

    rcu_ctl();

private:

    void switch_axis_wire(char);
};

