#pragma once

#include <vector>
#include <string>

#include <drivers/cnc-driver.h>

namespace we 
{
    class hardware;
    class axis_cfg;
}

class a_centering final
{
    typedef void (a_centering::*handler_t)();

public:

    a_centering(we::hardware &, we::axis_cfg const &) noexcept;

public:

    bool init_tooth(int, float) noexcept;

    bool init_shaft() noexcept;

    bool touch() noexcept;

    bool touch(handler_t) noexcept;

    bool is_error() const noexcept { return !emsg_.empty(); }

    bool is_done() const noexcept { return state_ == nullptr; }

    std::string const& emsg() const noexcept { return emsg_; }

    char proc_axis() const noexcept { return proc_axis_; }

private:

    bool init() noexcept;

    void set_error(std::string const&) noexcept;

    void next_state(handler_t nstate) noexcept { state_ = nstate; }

public:

    void limit_search_done() noexcept;

    void move_to_point_done() noexcept;

private:

    void check_touch_ctrl_active() noexcept;

    void init_next_axis() noexcept;

    void wait_reset_bki() noexcept;

    void wait_reset_bki_and_done() noexcept;

    void switch_to_independent_mode() noexcept;

    void limit_search_move() noexcept;

    void wait_limit_search_done() noexcept;

    void switch_to_target_mode() noexcept;

    void move_to_point() noexcept;

    void wait_move_to_point_done() noexcept;

private:

    we::hardware &hw_;
    cnc_driver cnc_;
    we::axis_cfg const &axis_cfg_;

    handler_t state_{ nullptr };
    std::string emsg_;

    // Обработчик, который должен будет вызваться когда концевик найден
    handler_t on_limit_found_action_;

    struct task_t
    {
        char axis;
        std::vector<int> dir;
        std::vector<float> rpos;
    };
    std::vector<task_t> axis_task_;
    task_t task_;

    float limit_search_speed_;
    float start_point_pos_;
    float move_to_pos_;

    float tooth_shift_;

    char proc_axis_;
};
