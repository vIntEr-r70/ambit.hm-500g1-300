#pragma once

#include <eng/sibus/node.hpp>

#include <bitset>

class working_mode_selector final
    : public eng::sibus::node
{
    std::bitset<2> selected_;

    std::array<eng::sibus::output_port_id_t, 4> out_;
    eng::sibus::output_port_id_t mode_out_;

    std::optional<std::size_t> current_selected_;

public:

    working_mode_selector();

private:

    void update_output();
};
