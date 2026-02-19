#pragma once

#include <eng/sibus/node.hpp>

#include <bitset>

class error_mask final
    : public eng::sibus::node
{
    std::bitset<3> ebits_;

    eng::sibus::output_port_id_t error_;
    bool result_{ false };

public:

    error_mask();

private:

    void update_output(bool);
};
