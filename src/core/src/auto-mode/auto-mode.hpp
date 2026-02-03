#pragma once

#include "vm.hpp"
#include "common/internal-state-ctl.hpp"

#include <eng/sibus/node.hpp>
#include <eng/timer.hpp>

class auto_mode final
    : public eng::sibus::node
    , public internal_state_ctl<auto_mode>
{
    typedef void (auto_mode::*command_handler)(eng::abc::pack);

    // Входящее управление
    eng::sibus::input_wire_id_t ictl_;

    // Управление движением
    eng::sibus::output_wire_id_t axis_ctl_;
    // Управление всем остальным
    eng::sibus::output_wire_id_t stuff_ctl_;

    eng::sibus::output_port_id_t phase_id_out_;

    vm vm_;
    std::vector<VmPhase*> phases_;

    std::vector<char> target_axis_;

    // eng::stopwatch stopwatch_;

    std::string program_b64_;

    // Список осей и их позиций
    std::unordered_map<char, double> axis_real_pos_;
    std::unordered_map<char, double> axis_initial_pos_;
    std::unordered_map<char, double> axis_program_pos_;

    eng::timer::id_t pause_timer_;

public:

    auto_mode();

private:

    void activate();

    void deactivate();

private:

    void s_wait_system_ready();

    void s_initialize();

    void s_program_execution_loop();

    void s_init_pause();

    void s_infinity_pause();

    void s_pause_done();

    void s_pause();

    void s_execute_operation();

    void s_start_moving();

    void s_wait_moving_done();

private:

    void execute_operation();

    bool create_and_execute_move_task();

    void update_program_phase();

    void create_and_execute_stuff_task();

private:

    void upload_program(eng::abc::pack);

    bool prepare_program(bool&, std::array<bool, 3> &, std::vector<char> &);

    void create_operation(struct program const&, std::size_t, bool &, std::array<bool, 3> &) noexcept;

private:

    bool prepare_node_for_work();

private:

    void load_axis_list();

private:


private:

    void register_on_bus_done() override final;
};

