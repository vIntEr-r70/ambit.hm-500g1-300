#pragma once

#include "slaves/motion-control.hpp"

#include <eng/log.hpp>

#include <cmath>
#include <vector>
#include <optional>
#include <array>
#include <algorithm>
#include <chrono>

template <typename T>
class position_motion final
    : public motion_control
{
    T &ctl_;

    // Текущая скорость движения
    double speed_limit_{ 0.0 };
    double acc_{ 0.0 };

    struct phase_t
    {
        double t;
        union
        {
            double a;
            double v;
        };
    };
    using task_t = std::array<phase_t, 3>;

    std::vector<task_t> phases_;

    std::optional<double> target_position_;
    std::optional<double> relative_position_;

public:

    void update_absolute_target(double value)
    {
        target_position_ = value;
        phases_.clear();
    }

    void update_relative_target(double value)
    {
        if (relative_position_)
            value += *relative_position_;
        relative_position_ = value;
        phases_.clear();
    }

    void update_speed_limit(double value)
    {
        speed_limit_ = value;
        phases_.clear();
    }

public:

    position_motion(T &ref)
        : ctl_(ref)
    { }

private:

    double calculate_phases(double v0, double v1, double distance)
    {
        if (v0 != 0.0 && std::signbit(v0) != std::signbit(v1))
            v1 = 0.0;

        double acc = std::copysign(acc_, (v1 - v0));
        auto &p0 = phases_.back()[0];
        p0.t = (v1 - v0) / acc;
        p0.a = acc;
        double s0 = (v0 + v1) * p0.t * 0.5;
        distance -= s0;

        acc = std::copysign(acc_, (0.0 - v1));
        auto &p2 = phases_.back()[2];
        p2.t = (0.0 - v1) / acc;
        p2.a = acc;
        double s2 = (0.0 + v1) * p2.t * 0.5;
        distance -= s2;

        if (v1 != 0.0 && distance != 0.0)
        {
            if (std::signbit(distance) == std::signbit(v1))
            {
                auto &p1 = phases_.back()[1];
                p1.t = distance / v1;
                double s1 = p1.t * v1;
                p1.v = v1;
                distance -= s1;
            }
        }

        auto &p1 = phases_.back()[1];
        double s1 = p1.t * p1.v;

        return distance;
    }

    void calculate_phases(double distance)
    {
        double v0 = v0_;

        for (std::size_t i = 0; i < 2; ++i)
        {
            phases_.emplace_back();

            double vmax = std::sqrt((acc_ * std::abs(distance)) + (v0 * v0 * 0.5));
            double vlim = std::copysign(std::min(vmax, speed_limit_), distance);
            // std::println("[{}] -> vmax: {:.3f}, vlim: {:.3f}", i, vmax, vlim);

            distance = calculate_phases(v0, vlim, distance);
            // std::println("[{}] -> distance: {}", i, distance);

            if (distance == 0.0)
                break;

            v0 = 0.0;
        }
    }

private:

    bool is_active() const noexcept override final
    {
        if (!phases_.empty())
            return true;
        ctl_.motion_done(true);
        return false;
    }

    void do_hard_stop() const noexcept override final
    {
        ctl_.motion_done(false);
    }

    // Возвращаем
    double next_position(double position, double dt) noexcept override final
    {
        // Если обновилась уставка ускорения
        if (motion_control::acc_)
        {
            acc_ = *motion_control::acc_;
            motion_control::acc_.reset();
            phases_.clear();
        }

        // Необходимо убедиться что параметры движения рассчитаны
        if (phases_.empty())
        {
            if (!target_position_)
                target_position_ = position;

            if (relative_position_)
            {
                *target_position_ += *relative_position_;
                relative_position_.reset();
            }

            double distance = *target_position_ - position;
            if (distance == 0.0) return position;

            calculate_phases(distance);
        }

        std::array<double, 3> S{ 0.0, 0.0, 0.0 };

        while (!phases_.empty())
        {
            auto &front = phases_.front();

            // Время и расстояние при разгоне
            {
                auto &phase = front[0];
                if (phase.t > 0.0 && dt > 0.0)
                {
                    double t = std::min(phase.t, dt);
                    phase.t -= t; dt -= t;

                    S[0] += (2 * v0_ + t * phase.a) * t * 0.5;
                    v0_ += t * phase.a;
                }
            }

            // Время постоянного движения
            {
                auto &phase = front[1];
                if (phase.t > 0.0 && dt > 0.0)
                {
                    double t = std::min(phase.t, dt);
                    phase.t -= t; dt -= t;

                    S[1] += phase.v * t;
                    v0_ = phase.v;
                }
            }

            {
                auto &phase = front[2];
                if (phase.t > 0.0 && dt > 0.0)
                {
                    double t = std::min(phase.t, dt);
                    phase.t -= t; dt -= t;

                    S[2] += (2 * v0_ + t * phase.a) * t * 0.5;
                    v0_ += t * phase.a;
                }
            }

            // Если после выполнения схемы у нас осталось время,
            // переходим к следующей схеме удалив предидущую
            if (dt <= 0.0)
                break;

            phases_.erase(phases_.begin());
        }

        return position + (S[0] + S[1] + S[2]);
    }
};

template <typename T>
class velocity_motion final
    : public motion_control
{
    T &ctl_;

    // Текущая скорость движения
    double speed_limit_{ 0.0 };
    double acc_{ 0.0 };

public:

    double target_speed;
    double acceleration;

public:

    velocity_motion(T &ref)
        : ctl_(ref)
    { }

public:

    void update_speed_limit(double value)
    {
        speed_limit_ = value;
    }

private:

    bool is_active() const noexcept override final
    {
        if (v0_ != 0.0)
            return true;
        ctl_.motion_done(true);
        return false;
    }

    void do_hard_stop() const noexcept override final
    {
        ctl_.motion_done(false);
    }

    double next_position(double position, double dt) noexcept override final
    {
        // Если обновилась уставка ускорения
        if (motion_control::acc_)
        {
            acc_ = *motion_control::acc_;
            motion_control::acc_.reset();
        }

        // Необходимо двигаться с ускорением
        if (v0_ != speed_limit_)
        {
            // Ускорение
            double acc = (v0_ > speed_limit_) ? acc_ : -acc_;

            double v0 = v0_;
            v0_ += acc * dt;

            if ((acc > 0) == (v0_ >= speed_limit_))
                v0_ = speed_limit_;

            return position + (v0_ + v0) * dt * 0.5;
        }

        return position + v0_ * dt;
    }
};

template <typename T>
class timed_motion final
    : public motion_control
{
    T &ctl_;

    double t0_;
    double s0_;

    struct interval_t
    {
        double acc;
        double dt;
        double dS;

        std::array<double, 3> idt;
        std::array<double, 3> ids;
        std::array<double, 3> iv;
    };
    std::vector<interval_t> intervals_;

    std::size_t iid_;
    bool in_proc_;
    bool terminate_;

    struct move_info_t
    {
        double a;

        double t1;
        double t2;
        double t3;

        double v;
        double v1;

        double ds;
    };

public:

    timed_motion(T &ref)
        : ctl_(ref)
    { }

public:

    void init(double t0)
    {
        in_proc_ = false;
        intervals_.clear();
        terminate_ = false;

        t0_ = t0;
        iid_ = 0;
    }

    // Удаляем еще не отработанные интервалы
    void reset()
    {
        terminate_ = true;
    }

    void add_interval(move_info_t const &mi)
    {
        // Начальная скорость это конечная скорость
        // предидущего интервала
        double v0 = intervals_.empty() ?
            0.0 : intervals_.back().iv[2];

        intervals_.emplace_back();
        auto &i = intervals_.back();

        i.acc = mi.a;
        i.dt = mi.t1 + mi.t2 + mi.t3;
        i.dS = mi.ds;

        i.idt = { mi.t1, mi.t2, mi.t3 };
        i.iv = { v0, mi.v, mi.v1 };

        // Считаем пройденные на каждом этапе расстояния
        i.ids = {
            v0 * mi.t1 + mi.a * mi.t1 * mi.t1 * 0.5,
            mi.v * mi.t2,
            mi.v1 * mi.t3 + mi.a * mi.t3 * mi.t3 * 0.5,
        };

        std::ranges::for_each(i.ids, [&](double &v) { v = std::copysign(v, i.dS); });
    }

private:

    bool is_active() const noexcept override final
    {
        if (iid_ < intervals_.size())
            return true;
        ctl_.motion_done(true);
        return false;
    }

    void do_hard_stop() const noexcept override final
    {
        ctl_.motion_done(false);
    }

    double next_position(double position, double) noexcept override final
    {
        // Текущий момент времени
        double now_time = steady_time();

        if (!in_proc_)
        {
            if (terminate_)
            {
                intervals_.clear();
                return position;
            }

            // Время начала движения еще не настало
            if (t0_ > now_time)
                return position;

            in_proc_ = true;
            s0_ = position;
        }

        // Текущий выполняемый интервал
        auto &interval = intervals_[iid_];

        double t0 = t0_;
        double s0 = s0_;

        for (std::size_t i = 0; i < 3; ++i)
        {
            // Проверяем, на каком этапе мы находимся
            if (t0 + interval.idt[i] <= now_time)
            {
                t0 += interval.idt[i];
                s0 += interval.ids[i];
                continue;
            }

            double dt = now_time - t0;
            double v = interval.iv[i];

            switch (i)
            {
            // Этап разгона
            case 0:
            // Этап замедления
            case 2: {
                double ds = (v * dt + interval.acc * dt * dt * 0.5 * (i ? -1.0 : 1.0));
                return s0 + std::copysign(ds, interval.dS);
            }
            // Этап движения с постоянной скоростью
            case 1: {
                double ds = v * dt;
                // Если выполнение было прервано, делаем
                // длительность этапа текущим моментом времени
                if (terminate_)
                {
                    interval.idt[i] = dt;
                    interval.ids[i] = ds;
                }
                // Рассчитываем новую координату
                return s0 + std::copysign(ds, interval.dS); }
            }
        }

        // Мы преодалели текущий этап, переключаемся на следующий
        if (terminate_)
        {
            iid_ = 0;
            intervals_.clear();
        }
        else
        {
            iid_ += 1;
        }

        // Смещаем точку отсчета
        t0_ = t0;
        s0_ = s0;

        return s0_;
    }

private:

    double steady_time() const noexcept
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = now.time_since_epoch();
        return elapsed_seconds.count();
    }
};

