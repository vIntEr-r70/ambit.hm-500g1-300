#pragma once

#include <eng/sibus/node.hpp>

class drainage_ctl final
    : public eng::sibus::node
{
    eng::sibus::output_port_id_t pump_;
    void (drainage_ctl::*pump_state_)();

    std::optional<bool> pmin_;
    std::optional<bool> pmax_;

    eng::sibus::output_wire_id_t va_ctl_;

    enum class vstate
    {
        drainage,
        barrels,
    };
    // vstate real_state_{ vstate::transition };

    vstate target_valve_state_;
    std::optional<vstate> next_valve_state_;

    void (drainage_ctl::*valve_state_)(vstate);

public:

    drainage_ctl();

private:

    void turned_off();

    void turned_on();

private:

    void set_valve_position(vstate);

    void update_valve_state(eng::sibus::istatus);

    void valve_move_to_drainage(vstate);

    void valve_move_to_barrels(vstate);

    void valve_in_drainage(vstate);

    void valve_in_barrels(vstate);
};

