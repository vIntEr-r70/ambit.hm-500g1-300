#pragma once

#include <eng/sibus/node.hpp>

class drainage_ctl final
    : public eng::sibus::node
{
public:

    drainage_ctl();

// private:
//
//     void on_activate() noexcept override final;
//
//     void on_deactivate() noexcept override final;

private:

    void wait_state_changed();

    void turn_on_off_pump();

    void turn_on_off_pump_error();

private:

    // std::string sid_;
    // we::hardware &hw_;
    //
    // std::optional<bool> pump_;
    // bool pump_on_off_;
    //
    // engine::bit_driver H7_ctl_;
};

