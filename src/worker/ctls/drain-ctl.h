#pragma once

#include <state-ctrl.h>
#include <drivers/reg-driver.h>
#include <drivers/bit-driver.h>

#include <optional>

namespace we {
    class hardware;
}

class drain_ctl final
    : public state_ctrl<drain_ctl>
{
public:

    drain_ctl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void init_unit();
    
    void init_unit_error();

    void wait_new_position();

    void do_target_pos();

    void do_VA10();
    
    void do_VA11();
    
    void do_target_pos_error();
    
private:
    
    void update_state(bool, bool) noexcept;
    
private:

    std::string sid_;
    we::hardware &hw_;

    bool drain_{ false };
    bool tank_{ false };

    std::optional<std::size_t> position_;
    std::size_t target_pos_;
    
    engine::reg_driver pos_ctl_;
    engine::reg_driver timeout_ctl_;

    engine::bit_driver VA10_ctl_;
    engine::bit_driver VA11_ctl_;
    
    struct init_step_t
    {
        engine::reg_driver *driver;
        std::uint16_t value;
        std::string message;
    };
    std::vector<init_step_t> init_steps_;
    std::size_t init_step_;

    std::uint32_t status_;
    bool status_ready_{ false };
    
    std::uint16_t state_;
    bool state_ready_{ false };
};

