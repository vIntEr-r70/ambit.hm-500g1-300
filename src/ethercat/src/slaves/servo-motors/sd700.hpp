#pragma once

#include "slaves/servo-motor.hpp"
#include "value-holder.hpp"

#include <cstdint>

class sd700 final
    : public servo_motor
{
    value_holder_rw<std::int8_t> working_mode_;
    value_holder_rw<std::uint16_t> control_word_;
    value_holder_rw<std::int32_t> target_speed_;
    value_holder_rw<std::int32_t> target_pos_;

    value_holder_ro<std::uint16_t> status_word_;
    value_holder_ro<std::uint16_t> error_code_;
    value_holder_ro<std::uint32_t> real_pos_;

public:

    sd700(slave_target_t);

private:

    void set_target_pos(std::int32_t) override final;

    void set_target_speed(std::int32_t) override final;

    servo_motor_status get_status() const override final;

    void set_control_word(std::uint16_t) override final;

    void set_control_mode(control_mode) override final;

    std::int32_t real_pos() const override final;

    void activate_probe() override final;
};
