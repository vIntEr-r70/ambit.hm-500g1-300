#include "base-ctrl.h"

#include <hardware.h>

base_ctrl::base_ctrl(std::string_view sid, we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : hw_(hw)
    , axis_cfg_(axis_cfg)
{ 
    hw.LSET.add(sid, "speed-bit0", [this](nlohmann::json const &value) {
        update_speed(0, value.get<bool>());
    });
    hw.LSET.add(sid, "speed-bit1", [this](nlohmann::json const &value) {
        update_speed(1, value.get<bool>());
    });
}

void base_ctrl::axis_cfg_changed() noexcept
{
    axis_cfg_changed_ = true;
}

void base_ctrl::update_speed(std::size_t id, bool state) noexcept
{
    if (state)
        speed_tsp_ |= (1 << id);
    else
        speed_tsp_ &= ~(1 << id);
    
    speed_tsp_ = std::min<std::size_t>(speed_tsp_, 2);
}

void base_ctrl::update_internal_axis_cfg() noexcept
{
    axis_cfg_changed_ = false;
    real_speed_tsp_ = speed_tsp_;

    for (auto const& axis : axis_cfg_)
    {
        if (!axis.second.use())
            continue;
        float speed = get_speed(axis.second, real_speed_tsp_);
        speed_[axis.first] = speed;
        
        hw_.notify({ "cnc", std::string(1, axis.first), "speed" }, speed);
    }
}

bool base_ctrl::is_new_axis_cfg() const noexcept
{
    return (speed_tsp_ != real_speed_tsp_) || axis_cfg_changed_;
}

void base_ctrl::check_new_axis_cfg() noexcept
{
    if (!is_new_axis_cfg())
        return;
    update_internal_axis_cfg();
}


