#pragma once

#include "actors/a_centering.h"
#include "common/Defs.h"

#include <state-ctrl.h>
#include <drivers/cnc-driver.h>

#include <aem/countdown.h>

class centering_ctrl final
    : public state_ctrl<centering_ctrl>
{
public:

    centering_ctrl(we::hardware &, we::axis_cfg const &) noexcept;

public:

    void sync_state() noexcept;

    centering_type ctype() const noexcept { return ctype_; }
    
private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

    void on_deactivated() noexcept override final;

private:

    void do_search_steps() noexcept;

    void do_move_steps() noexcept;

    void terminate_independent_move() noexcept;

    void terminate_target_move() noexcept;

    void wait_terminate_target_move_stop() noexcept;

    void reset_queue() noexcept;

    void centering_done() noexcept;

    void centering_error() noexcept;

private:

    void handle_error() noexcept;

    void init_step() noexcept;

    void next_step() noexcept;

    void step_error() noexcept;

private:

    float shift_;
    centering_type ctype_{ centering_type::not_active };

    we::hardware &hw_;

    int step_{ 0 };

    a_centering a_centering_;

    cnc_driver cnc_;
};
