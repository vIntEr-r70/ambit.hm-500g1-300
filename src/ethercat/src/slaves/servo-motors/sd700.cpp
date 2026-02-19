#include "sd700.hpp"
#include "slaves/servo-motor.hpp"

#include <bitset>

#include "ethercat.hpp"

#include <print>

sd700::sd700(slave_target_t target)
    : servo_motor({ .target = target, .VendorID = 0x00850104, .ProductCode = 0x01030507 })
{
    status_word_.on_changed([this, target]
    {
        std::bitset<16> bits(status_word_.get());
        std::println("[{}] >> StatusWord: {}", target.position, bits.to_string());
    });

    error_code_.on_changed([this, target]
    {
        std::println("[{}] >> ErrorCode: {:04X}", target.position, error_code_.get());
    });

    ethercat::pdo::add(this, 0x6040, 0x00, control_word_);
    ethercat::pdo::add(this, 0x6060, 0x00, working_mode_);
    ethercat::pdo::add(this, 0x607A, 0x00, target_pos_);

    ethercat::pdo::add(this, 0x6041, 0x00, status_word_);
    ethercat::pdo::add(this, 0x603f, 0x00, error_code_);
    ethercat::pdo::add(this, 0x6064, 0x00, real_pos_);
}

void sd700::set_target_pos(std::int32_t value)
{
    // Просто задаем текущую позицию драйверу
    target_pos_.set(value);
}

std::int32_t sd700::real_pos() const
{
    return real_pos_.get();
}

servo_motor_status sd700::get_status() const
{
    std::bitset<16> bits(status_word_.get());

    // Servo no fault
    if (!bits.test(0) && error_code_.get() != 0)
        return servo_motor_status::fault;

    // Servo operation
    if (bits.test(1) && bits.test(2) && bits.test(4) && bits.test(5))
        return servo_motor_status::running;

    // Waiting to turn on the servo enable
    if (bits.test(1) && bits.test(4) && bits.test(5))
        return servo_motor_status::enable;

    // Turn on the main loop // Rapid shutdown
    if (bits.test(4) && bits.test(5))
        return servo_motor_status::ready;

    return servo_motor_status::fault_free;
}

void sd700::set_control_word(std::uint16_t ControlWord)
{
    if (control_word_.get() != ControlWord)
    {
        std::bitset<16> bits(ControlWord);
        std::println("[{}] << ControlWord: {}", info().target.position, bits.to_string());
    }
    control_word_.set(ControlWord);
}

void sd700::set_control_mode(control_mode mode)
{
    static std::unordered_map<control_mode, std::int8_t> const modes{
        { control_mode::none,   0 },
        { control_mode::pv,     3 },
        { control_mode::csp,    8 }
    };

    auto it = modes.find(mode);
    if (it == modes.end())
    {
        std::println("sd700::set_control_mode: unknown mode");
        return;
    }

    working_mode_.set(it->second);
}

std::uint32_t sd700::DI() const
{
    return 0;
}

std::uint16_t sd700::get_raw_status() const
{
    return 0;
}
