#pragma once

#include "ethercat-slave.hpp"
#include "types.hpp"

#include <cmath>
#include <optional>

enum class servo_motor_status
{
    fault,
    running,
    enable,
    ready,
    fault_free
};

class motion_control;

class servo_motor
    : public ethercat_slave
{
    void (servo_motor::*mode_)();
    void (servo_motor::*motion_)(double);

    control_mode control_mode_{ control_mode::csp };
    double time_point_{ NAN };

    motion_control *ctl_{ nullptr };

    double acc_limit_{ 1.0 };

    std::optional<double> cfg_ratio_;
    double ratio_{ 1.0 };

    double position_{ NAN };

protected:

    servo_motor(slave_info_t);

public:

    void set_control(motion_control &) noexcept;

    motion_control* control() const noexcept { return ctl_; }

    void set_acc_limit(double) noexcept;

    void set_ratio(double value) noexcept { cfg_ratio_ = value; }

    double acc_limit() const noexcept { return acc_limit_; }

    double position() const noexcept;

    double speed() const noexcept;

private:

    void update(double) override;

private:

    void fault_reset_0();
    void fault_reset_1();
    void fault_reset_2();

    void turning_on();

    void turning_off();

    void running();

private:

    void control_mode_csp(double);

private:

    virtual void set_target_pos(std::int32_t) = 0;

    virtual void set_target_speed(std::int32_t) = 0;

    virtual servo_motor_status get_status() const = 0;

    virtual void set_control_word(std::uint16_t) = 0;

    virtual void set_control_mode(control_mode) = 0;

    virtual std::int32_t real_pos() const = 0;

    virtual void activate_probe() = 0;
};

