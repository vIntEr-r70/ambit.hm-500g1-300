#pragma once

#include "motion-controls.hpp"

#include <eng/sibus/node.hpp>

class servo_motor;

class axis_motion_node final
    : public eng::sibus::node
{
    using commands_handler = bool(axis_motion_node::*)(eng::abc::pack const &);

    // Ось, которой мы управляем
    char axis_;
    servo_motor &servo_motor_;

    eng::sibus::input_wire_id_t ictl_;

    eng::sibus::output_port_id_t position_;
    eng::sibus::output_port_id_t speed_;
    eng::sibus::output_port_id_t status_;

    position_motion<axis_motion_node> by_position_;
    velocity_motion<axis_motion_node> by_velocity_;
    timed_motion<axis_motion_node> by_time_;

    double origin_offset_{ 0.0 };

public:

    axis_motion_node(char, servo_motor &);

public:

    bool motion_done();

    void motion_status();

private:

    bool cmd_move_to(eng::abc::pack const &);

    bool cmd_shift(eng::abc::pack const &);

    bool cmd_spin(eng::abc::pack const &);

    bool cmd_stop(eng::abc::pack const &);

    bool cmd_zerro(eng::abc::pack const &);

    bool cmd_timed_shift(eng::abc::pack const &);

private:

    double local_position() const noexcept;

    void update_output_info();

private:

    void start_command_execution(eng::abc::pack);

private:

    void prepare_work_mode();

    void deactivate();

private:

    void register_on_bus_done() override final;
};
