#pragma once

#include <eng/countdown.hpp>
#include <eng/abc/pack.hpp>

#include <vector>
#include <unordered_map>

#include "common/program-phases.hpp"

class vm
{
    static constexpr std::size_t MAX_AXIS_COUNT{ 10 };

    std::vector<struct VmPhase*> const &phases_;
    std::vector<char> const &target_axis_;

    struct axis_distance_t
    {
        char axis;
        double value;
    };

    struct phase_axis_segment_t
    {
        char axis;
        double from;
        double to;
    };

    struct phase_moving_info_t
    {
        // Номер этапа
        std::size_t phase_id;

        // Смещения осей
        axis_distance_t distance[MAX_AXIS_COUNT];
        phase_axis_segment_t segments[MAX_AXIS_COUNT];
        std::size_t axis_count;

        // Скорость движения
        double speed;
    };
    std::vector<phase_moving_info_t> continuous_moving_list_;

    std::unordered_map<VmOpType, void(vm::*)(VmOperation::Item const*, bool)> map_pos_operations_;

    struct axis_info_t
    {
        double position;
        double next_position;
        int moving_direction;
    };
    std::unordered_map<char, axis_info_t> phase_moving_;

    // Скорость перемещения
    double phase_motion_speed_;
    // Скорость перемещения
    double next_phase_motion_speed_;

    std::size_t phase_id_;
    std::unordered_map<std::size_t, std::size_t> known_goto_;
    std::vector<std::size_t> dec_N_goto_pid_;
    std::vector<std::size_t> dec_known_goto_pid_;

    std::vector<std::size_t> ops_phases_;

public:

    vm(std::vector<VmPhase*> const &, std::vector<char> const &) noexcept;

public:

    void initialize();

    bool is_program_done() const;

    VmPhaseType phase_type() const;

    std::size_t to_next_phase();

    std::size_t phase_id() const noexcept;

    bool has_phase_ops() const noexcept { return !ops_phases_.empty(); }

    std::uint64_t pause_timeout_ms() const;

    bool check_in_position(std::unordered_map<char, double> const &) const;

    bool create_continuous_moving_list();

    void fill_cnc_task(eng::abc::pack &) const;

    void fill_stuff_task(std::size_t, eng::abc::pack &) const;

private:

    bool try_add_next_phase(std::size_t &);

    VmPhase const* get_next_operation(std::size_t &) noexcept;

private:

    void fill_operation_task(VmOperation const &, eng::abc::pack &) const;

private:

    void _vm_next_op_pos(VmOperation::Item const*, bool) noexcept;
    void _vm_next_op_speed(VmOperation::Item const*, bool) noexcept;
};

