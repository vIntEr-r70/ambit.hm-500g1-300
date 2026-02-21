#pragma once

#include <eng/sibus/node.hpp>

class manual_auto_switch final
    : public eng::sibus::node
{
    eng::sibus::output_port_id_t manual_;
    eng::sibus::output_port_id_t auto_;

    eng::sibus::output_port_id_t mode_;

public:

    manual_auto_switch();

private:

    void update_output(eng::abc::pack);
};
