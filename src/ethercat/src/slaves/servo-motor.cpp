#include "servo-motor.hpp"
#include "motion-control.hpp"

#include <eng/log.hpp>

#include <cmath>

servo_motor::servo_motor(slave_info_t target)
    : ethercat_slave(target)
    , mode_{ &servo_motor::turning_off }
    , motion_{ nullptr }
{ }

void servo_motor::set_control(motion_control &ctl) noexcept
{
    ctl_ = &ctl;

    if (!std::isnan(acc_limit_))
        ctl_->set_acc(acc_limit_);
}

void servo_motor::set_acc_limit(double value) noexcept
{
    acc_limit_ = value;

    if (ctl_ != nullptr)
        ctl_->set_acc(acc_limit_);
}

// Вызывается драйвером ethercat каждую итерацию цикла
void servo_motor::update(double seconds)
{
    set_control_mode(control_mode_);

    if (std::isnan(time_point_))
        time_point_ = seconds;

    (this->*mode_)();

    if (motion_ && ctl_)
        (this->*motion_)(seconds - time_point_);

    time_point_ = seconds;
}

void servo_motor::turning_on()
{
#ifdef BUILDROOT
    std::int32_t rpos = real_pos();
    set_target_pos(rpos);

    if (cfg_ratio_)
    {
        ratio_ = *cfg_ratio_;
        eng::log::info("RATIO[{}]: {}", info().target.position, ratio_);
        cfg_ratio_.reset();
    }

    if (!std::isnan(ratio_))
        position_ = rpos / ratio_;

    eng::log::info("servo_motor[{}]::turning_on: real-pos = {}, ratio = {}, position = {}, new-real-pos = {}",
            info().target.position, rpos, ratio_, position_, std::lround(position_ * ratio_));

    switch(get_status())
    {
    case servo_motor_status::fault:
        mode_ = &servo_motor::fault_reset_0;
        eng::log::info("-> servo_motor[{}]::fault_reset_0", info().target.position);
        return;
    case servo_motor_status::fault_free:
        set_control_word(0b0110);
        return;
    case servo_motor_status::ready:
        set_control_word(0b0111);
        return;
    case servo_motor_status::enable:
        set_control_word(0b1111);
        return;
    case servo_motor_status::running:
        break;
    }
#else
    position_ = 0.0;
#endif

    eng::log::info("-> servo_motor[{}]::control_mode", info().target.position);

    mode_ = &servo_motor::running;
    motion_ = &servo_motor::control_mode_csp;
}

void servo_motor::turning_off()
{
#ifdef BUILDROOT
    switch(get_status())
    {
    case servo_motor_status::fault:
        mode_ = &servo_motor::fault_reset_0;
        return;
    case servo_motor_status::running:
        set_control_word(0b0111);
        return;
    case servo_motor_status::enable:
        set_control_word(0b0110);
        return;
    case servo_motor_status::ready:
        set_control_word(0b0000);
        return;
    case servo_motor_status::fault_free:
        break;
    }
#endif

    mode_ = &servo_motor::turning_on;
    eng::log::info("-> servo_motor[{}]::turning_on", info().target.position);
}

void servo_motor::fault_reset_0()
{
    switch(get_status())
    {
    case servo_motor_status::fault:
        set_control_word(0b10000000);
        return;
    default:
        break;
    }

    mode_ = &servo_motor::fault_reset_1;

    eng::log::info("-> servo_motor[{}]::fault_reset_1", info().target.position);
}

void servo_motor::fault_reset_1()
{
    switch(get_status())
    {
    case servo_motor_status::fault:
        set_control_word(0b00000000);
        return;
    default:
        break;
    }

    mode_ = &servo_motor::fault_reset_2;

    eng::log::info("-> servo_motor[{}]::fault_reset_2", info().target.position);
}

void servo_motor::fault_reset_2()
{
    switch(get_status())
    {
    case servo_motor_status::fault:
        set_control_word(0b10000000);
        return;
    default:
        break;
    }

    mode_ = &servo_motor::turning_off;

    eng::log::info("-> servo_motor[{}]::turning_off", info().target.position);
}

void servo_motor::running()
{
#ifdef BUILDROOT
    if (control_mode_ == control_mode::csp)
    {
        switch(get_status())
        {
        case servo_motor_status::running:
            return;
        default:
            break;
        }
    }

    mode_ = &servo_motor::turning_off;
    motion_ = nullptr;

    eng::log::error("-> servo_motor[{}]::turning_off by fault", info().target.position);
#endif
}

void servo_motor::control_mode_csp(double dt)
{
    if (ctl_ == nullptr)
    {
        if (cfg_ratio_)
        {
            ratio_ = *cfg_ratio_;
            eng::log::info("RATIO[{}]: {}", info().target.position, ratio_);
            cfg_ratio_.reset();
        }

#ifdef BUILDROOT
        std::uint32_t rpos = real_pos();
#else
        std::uint32_t rpos = 0;
#endif

        if (!std::isnan(ratio_))
            position_ = rpos / ratio_;

        return;
    }

    if (!std::isnan(ratio_) && !std::isnan(position_))
    {

        position_ = ctl_->next_position(position_, dt);
#ifdef BUILDROOT
        set_target_pos(std::lround(position_ * ratio_));
#endif
    }

    // // Убеждаемся что нам все еще имеет смысл работать
    if (!ctl_->is_active())
    {
        eng::log::info("servo_motor::control_mode_csp: NO ACTIVE");
        ctl_ = nullptr;
    }
}

double servo_motor::position() const noexcept
{
    return position_;
}

double servo_motor::speed() const noexcept
{
    return ctl_ ? ctl_->speed() : 0.0;
}

