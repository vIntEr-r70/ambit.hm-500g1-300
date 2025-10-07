#pragma once

#include <state-ctrl.h>
#include <drivers/reg-driver.h>
#include <drivers/bit-driver.h>
#include <optional>

namespace we 
{
    class hardware;
}

class pr_barrel_ctl final
    : public state_ctrl<pr_barrel_ctl>
{
public:

    pr_barrel_ctl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void wait_new_tasks();

    void do_task();
    
    void do_task_error();

    void do_activate();

    void do_activate_error();

private:
    
    void update_temp_min();

    void update_temp_max();
    
private:

    std::string sid_;
    we::hardware &hw_;

    std::optional<bool> op_activate_;
    bool activate_{ false };
    
    engine::bit_driver activate_ctl_;
    
    engine::reg_driver add_liquid_ctl_;
    engine::reg_driver temp_min_ctl_;
    engine::reg_driver temp_max_ctl_;

    struct task_t
    {
        engine::reg_driver *driver;
        std::uint16_t value;
        std::string message;
    };
    std::vector<task_t> tasks_;
    std::size_t task_id_;
    
    float temp_min_;
    float temp_max_;
};

