#pragma once

#include <drivers/cnc-driver.h>
#include <map>

namespace we {
    class hardware;
}

class base_ctrl
{
protected:

    base_ctrl(std::string_view, we::hardware &, we::axis_cfg const &) noexcept;

public:

    virtual ~base_ctrl() = default;

public:

    void axis_cfg_changed() noexcept;

protected:

    virtual void update_internal_axis_cfg() noexcept;

    virtual bool is_new_axis_cfg() const noexcept;

    virtual float get_speed(we::axis_desc const &, std::size_t) const noexcept = 0;

    void check_new_axis_cfg() noexcept;

    void update_speed(std::size_t, bool) noexcept;

protected:

    we::hardware &hw_;
    we::axis_cfg const &axis_cfg_;

    // Положение тумблера выбора скорости на панели
    std::size_t speed_tsp_{ 0 };

    // Скорость, с которой в данном состоянии выполняются перемещения
    std::map<char, float> speed_;
    std::size_t real_speed_tsp_{ 0 };

private:

    bool axis_cfg_changed_{ false };
};

