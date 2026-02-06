#pragma once

#include <eng/sibus/node.hpp>

#include <bitset>

class current_conductors final
    : public eng::sibus::node
{
    struct record_t
    {
        std::string key;
        double I;
        double U;
    };
    std::vector<record_t> records_;

    std::bitset<2> selected_;

    eng::sibus::output_port_id_t p_out_;

public:

    current_conductors();

private:

    void update_output();
};
