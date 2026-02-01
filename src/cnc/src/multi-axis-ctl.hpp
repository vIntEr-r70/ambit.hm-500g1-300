#pragma once

#include "eng/sibus/sibus.hpp"
#include <eng/sibus/node.hpp>
#include <unordered_set>

class multi_axis_ctl final
    : public eng::sibus::node
{
    constexpr static std::size_t MAX_AXIS_COUNT{ 10 };

    eng::sibus::input_wire_id_t ictl_;

    struct axis_t
    {
        char axis;
        double distance;
    };

    struct task_t
    {
        axis_t axis[MAX_AXIS_COUNT];
        std::size_t axis_count;
        double speed;
    };

    std::vector<task_t> tasks_;

    struct move_t
    {
        double t_acc;
        double t_v;
        double t_dec;
        double ds;
        double v_total;
        double v_limit;
        double v;
        double v0;
        double v1;
        double a;
        double k;
    };

    // Команда для выполнения осью
    struct phase_t
    {
        std::size_t move_step;
        double start_time;
        std::vector<move_t> moves;
    };

    // Список этапов движения для оси
    // Этап выполняется осью, после
    // завершения оси передается следующий этап
    struct axis_task_list_t
    {
        bool stopped{ true };
        std::vector<phase_t> phases;
    };
    std::unordered_map<char, axis_task_list_t> axis_phases_;

    std::unordered_set<char> help_set_;

    // Максимальное время, затрачиваемое осью на
    // выполнение соответствующего шага движения
    std::vector<double> move_step_times_;

    struct axis_group_item_t
    {
        char axis;
        std::size_t phase_id;
    };

    struct axis_group_t
    {
        std::size_t moving_steps;
        std::vector<axis_group_item_t> axis;
    };
    std::vector<std::vector<axis_group_t>> axis_groups_;

    struct axis_info_t
    {
        double acc;
        eng::sibus::output_wire_id_t ctl;
    };
    std::unordered_map<char, axis_info_t> info_;

    std::unordered_map<char, std::size_t> in_proc_;

    std::unordered_map<char, std::size_t> help_phases_;
    std::vector<axis_group_item_t> help_axis_group_;

public:

    multi_axis_ctl();

private:

    void activate(eng::abc::pack);

    void deactivate();

private:

    void execute_phase(char);

    std::size_t fill_common_axis_list();

    std::optional<std::size_t> get_phase_id(char, std::size_t) const noexcept;

    std::optional<std::size_t> get_phase_id_v2(char, std::size_t) const noexcept;

    void calculate_moving(axis_group_t const &);

    void calculate_moving(axis_group_item_t, std::size_t);

    // void sync_timings(std::size_t);

    double phase_time_begin(std::size_t) const noexcept;

    std::size_t calculate_move_step(std::size_t);

    std::size_t calculate_axis_move_step(std::size_t, move_t &);

    std::size_t calculate_group_axis_move_step(std::size_t, move_t const &, move_t &);

private:

    void register_on_bus_done() override final;

    void response_handler(char, bool);

    void wire_status_was_changed(char);

    void create_moving_tasks(eng::abc::pack);

    void convert_tasks_to_phases();
};

