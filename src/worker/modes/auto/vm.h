#pragma once

#include "actors/a_centering.h"

#include <map>
#include <vector>
#include <unordered_map>

#include <aem/countdown.h>
#include <drivers/cnc-driver.h>
#include <drivers/fc-reg-driver.h>
#include <drivers/bit-driver.h>

#include "common/program-phases.h"

#include <state-ctrl.h>

namespace we 
{
    class hardware;
    class axis_cfg;
}

class vm
    : public state_ctrl<vm>
{
public:

    vm(we::hardware&, we::axis_cfg const &) noexcept;

public:

    bool set_program(std::vector<VmPhase*>&, std::vector<char> &, std::vector<char> &, bool, bool) noexcept;

public:

    bool is_in_infinity_pause() const noexcept;

    void do_continue() noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

    void on_deactivated() noexcept override final;

private:

    void turn_on_sprayers_pump() noexcept;
    void turn_off_sprayers_pump() noexcept;
    
    void set_axis_spin_mode() noexcept;

    void set_axis_target_mode() noexcept;

    void try_add_next_target() noexcept;

    void moving_to_target() noexcept;

    void finite_pause() noexcept;

    void infinite_pause() noexcept;

    void timed_fc_0() noexcept;
    void timed_fc_1() noexcept;
    void timed_fc_2() noexcept;

    void centering() noexcept;

    void phase_done() noexcept;

private:

    VmPhase const* get_next_operation(std::size_t) const noexcept;

private:

    void _vm_operation(VmPhase const*) noexcept;
    void _vm_pause(VmPhase const*) noexcept;
    void _vm_goto(VmPhase const*) noexcept;
    void _vm_timedfc(VmPhase const*) noexcept;
    void _vm_center_shaft(VmPhase const*) noexcept;
    void _vm_center_tooth(VmPhase const*) noexcept;

private:

    void _vm_op_pos(VmOperation::Item const*, bool) noexcept;
    void _vm_op_spin(VmOperation::Item const*, bool) noexcept;
    void _vm_op_speed(VmOperation::Item const*, bool) noexcept;
    void _vm_op_fc(VmOperation::Item const*, bool) noexcept;
    void _vm_op_sprayer(VmOperation::Item const*, bool) noexcept;

private:

    void _vm_next_op_pos(VmOperation::Item const*, bool) noexcept;
    void _vm_next_op_speed(VmOperation::Item const*, bool) noexcept;

private:

    void op_error() noexcept;

    void _cnc_target_move() noexcept;
    void _cnc_next_target_move() noexcept;

    void apply_sys_state() noexcept;

    void _update_spin() noexcept;

    void _update_sprayer_0() noexcept;
    void _update_sprayer_1() noexcept;

    void _update_fc_0() noexcept;
    void _update_fc_1() noexcept;

private:

    void stop_program() noexcept;

    void stop_moving() noexcept;
    void wait_target_move_stop() noexcept;
    void reset_queue() noexcept;

    void stop_spin() noexcept;

    void stop_fc() noexcept;

    void stop_sprayer() noexcept;

private:

    void set_op_error(std::string const&) noexcept;

public:

    bool fc_error_;
    bool fc_started_;

private:

    we::hardware &hw_;

    cnc_driver cnc_;
    engine::fc_reg_driver fc_;

    std::vector<struct VmPhase*> phases_;

    std::vector<char> spin_axis_;
    std::vector<char> target_axis_;

    std::map<VmPhaseType, void(vm::*)(VmPhase const*)> map_phases_;
    std::map<VmOpType, void(vm::*)(VmOperation::Item const*, bool)> map_operations_;
    std::map<VmOpType, void(vm::*)(VmOperation::Item const*, bool)> map_pos_operations_;

    std::size_t phase_id_;

    std::map<std::size_t, std::size_t> known_goto_;

    // Спрейер включить или выключить
    // engine::bit_driver pump_;
    std::array<engine::bit_driver, 3> sprayer_;
    std::array<bool, 3> sprayers_state_;
    std::size_t sprayer_id_;

    // Предидущая позиция
    std::map<char, float> from_position_;
    // Целевые позиции осей зависимого движения
    std::map<char, float> target_position_;
    // Целевые позиции осей зависимого движения следующего шага
    std::map<char, float> next_target_position_;

    // Последняя отправленная драйверу позиция
    std::map<char, float> last_position_;

    // Скорость перемещения
    float target_motion_speed_;

    // Целевые скорости осей независимого вращения
    std::unordered_map<char, float> spin_;

    // Счетчик времени
    aem::countdown countdown_;

    a_centering a_centering_;

    bool use_fc_;
    bool fc_powered_;
    bool use_sprayers_;

    std::array<aem::uint16, 2> tvmc_;
};

