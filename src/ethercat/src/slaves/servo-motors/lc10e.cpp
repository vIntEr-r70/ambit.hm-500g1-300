#include "lc10e.hpp"
#include "ethercat.hpp"

#include <eng/timer.hpp>
#include <eng/abc/lambda-traits.hpp>
#include <eng/log.hpp>

#include <bitset>

lc10e::lc10e(slave_target_t target)
    : servo_motor({ .target = target, .VendorID = 0x00000766, .ProductCode = 0x00000402 })
{
    status_word_.on_changed([this, target]
    {
        std::bitset<16> bits(status_word_.get());
        eng::log::info("[{}] >> StatusWord: {}", target.position, bits.to_string());
    });

    error_code_.on_changed([this, target]
    {
        eng::log::info("[{}] >> ErrorCode: {:04X}", target.position, error_code_.get());
    });

    DI_state_.on_changed([this, target]
    {
        eng::log::info("[{}] >> DI state: {:08X}", target.position, DI_state_.get());
    });

    ethercat::pdo::add(this, 0x6040, 0x00, control_word_);
    ethercat::pdo::add(this, 0x6060, 0x00, working_mode_);
    ethercat::pdo::add(this, 0x607A, 0x00, target_pos_);

    ethercat::pdo::add(this, 0x6041, 0x00, status_word_);
    ethercat::pdo::add(this, 0x603f, 0x00, error_code_);
    ethercat::pdo::add(this, 0x6064, 0x00, real_pos_);
    ethercat::pdo::add(this, 0x60FD, 0x00, DI_state_);
}

// 01B10000: 0000000110110001 0000000000000000
// 01310000: 0000000100110001 0000000000000000

void lc10e::set_target_pos(std::int32_t value)
{
    // Просто задаем текущую позицию драйверу
    target_pos_.set(value);
}

std::int32_t lc10e::real_pos() const
{
    return real_pos_.get();
}

// 5444: 0101010001000100
// 5443: 0101010001000011

servo_motor_status lc10e::get_status() const
{
    std::bitset<16> bits(status_word_.get());

    std::uint16_t error_code = error_code_.get();
    bool error_set =
        error_code != 0x0000 &&
        error_code != 0x5444 &&
        error_code != 0x5443;

    // Servo no fault
    if (!bits.test(0) && error_set)
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

void lc10e::set_control_word(std::uint16_t ControlWord)
{
    control_word_.set(ControlWord);
}

void lc10e::set_control_mode(control_mode mode)
{
    static std::unordered_map<control_mode, std::int8_t> const modes{
        { control_mode::none,   0 },
        { control_mode::pv,     3 },
        { control_mode::csp,    8 }
    };

    auto it = modes.find(mode);
    if (it == modes.end())
    {
        eng::log::error("le10c::set_control_mode: unknown mode");
        return;
    }

    working_mode_.set(it->second);
}

