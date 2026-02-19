#pragma once

#include <eng/sibus/node.hpp>

#include <optional>

class bki_ctl final
    : public eng::sibus::node
{
    eng::sibus::input_wire_id_t ictl_;

    eng::sibus::output_port_id_t reset_;
    eng::sibus::output_port_id_t not_allow_;
    eng::sibus::output_port_id_t bki_;

    bool cfg_bki_allow_{ true };
    std::optional<bool> wait_for_bki_activate_back_;

public:

    bki_ctl();

private:

    void update_bki_allow(bool);

    void register_on_bus_done() override final;
};
