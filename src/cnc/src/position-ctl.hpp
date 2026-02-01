#pragma once

#include <eng/sibus/node.hpp>
#include <eng/sibus/sibus.hpp>
#include <eng/timer.hpp>

#include <chrono>
#include <list>

class position_ctl final
    : public eng::sibus::node
{
    double speed_up_{ 1.0 };
    double speed_down_{ 1.0 };
    double max_speed_{ 10.0 };
    double position_{ 0.0 };

    std::chrono::time_point<std::chrono::steady_clock> time_start_;
    std::chrono::time_point<std::chrono::steady_clock> gtime_start_;

    eng::sibus::output_port_id_t port_mode_;
    eng::sibus::output_port_id_t port_position_;
    // eng::sibus::output_port_id_t port_status_;

    struct task_phase_t
    {
        double v;
        double t;
        double s;
    };

public:

    struct task_t
    {
        double position;
        double distance;

        // Разгон, начальная скорость
        task_phase_t speed_up;
        // Движение с максимальной скоростью
        task_phase_t vmax;
        // Остановка, конечная скорость
        task_phase_t speed_down;
    };

private:

    std::list<task_t> tasks_;

    eng::timer::id_t timer_id_;

public:

    position_ctl(char);

private:

    void start_motion(eng::abc::pack const &);

public:

    void add_position_task(char, double);

    void add_speed_task(char, double);

    void remove_front_tasks(std::size_t);

    void shift_task_start_time(double);

private:

    task_phase_t get_process_task_info() const;

    std::pair<double, double> update_speed_up_phase(double, double, double, task_t &);

    double update_vmax_phase(double, double, task_t &);

    std::pair<double, double> update_speed_down_phase(double, double, double, task_t &);

    double phase_time_elapsed() const noexcept;

    void optimize_movement();

    void update_position();

    double get_phase_offset(double);
};
