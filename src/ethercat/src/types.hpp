#pragma once

struct velocity final
{
    explicit velocity(double value) : v(value) { }
    double v;
};

struct acceleration final
{
    explicit acceleration(double value) : v(value) { }
    double v;
};

struct k_velocity final
{
    explicit k_velocity(double value) : v(value) { }
    double v;
};

struct k_position final
{
    explicit k_position(double value) : v(value) { }
    double v;
};

enum class control_mode
{
    none,
    pv,
    csp,
};

