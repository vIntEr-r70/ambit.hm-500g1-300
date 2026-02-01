#pragma once

#include <optional>

class motion_control
{
protected:

    double v0_{ 0.0 };
    std::optional<double> acc_;

public:

    virtual ~motion_control() = default;

public:

    virtual double next_position(double, double) noexcept = 0;

    virtual bool is_active() const noexcept = 0;

public:

    double speed() const noexcept { return v0_; }

    void set_acc(double value) noexcept { acc_ = value; }
};
