#pragma once

#include "vm.h"

#include <aem/stopwatch.h>

class auto_mode
    : public state_ctrl<auto_mode>
{
public:

    auto_mode(we::hardware &, we::axis_cfg const &) noexcept;

public:

    bool is_ctrl_enabled() const noexcept;

    void start_program() noexcept;

    void stop_program() noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void wait_deactivate() noexcept;

    void wait_program() noexcept;

    void program_in_proc() noexcept;

private:

    void create_operation(struct program const&, std::size_t) noexcept;

    void clear_phases() noexcept;

private:

    we::hardware &hw_;

    vm vm_;

    std::vector<VmPhase*> phases_;

    std::vector<char> spin_axis_;
    std::vector<char> target_axis_;

    bool use_fc_;
    bool use_sprayers_;

    aem::stopwatch stopwatch_;

    std::string program_b64_;
};
