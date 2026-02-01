#include "lc10e.hpp"
#include "ethercat.hpp"

#include <eng/timer.hpp>
#include <eng/abc/lambda-traits.hpp>

#include <bitset>

lc10e::lc10e(slave_target_t target)
    : servo_motor({ .target = target, .VendorID = 0x00000766, .ProductCode = 0x00000402 })
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

void lc10e::set_target_pos(std::int32_t value)
{
    // Просто задаем текущую позицию драйверу
    target_pos_.set(value);
}

void lc10e::set_target_speed(std::int32_t value)
{
    // Просто задаем текущую скорость драйверу
    // target_speed_.set(value);
}

std::int32_t lc10e::real_pos() const
{
    return real_pos_.get();
}

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
    if (control_word_.get() != ControlWord)
    {
        std::bitset<16> bits(ControlWord);
        // std::println("[{}] << ControlWord: {}", info().target.position, bits.to_string());
    }
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
        std::println("le10c::set_control_mode: unknown mode");
        return;
    }

    working_mode_.set(it->second);
}

// void lc10e::update_mode(std::string_view mode)
// {
//     std::println("{}: lc10e::update_mode: [{}]", name(), mode);
//
//     static std::unordered_map<std::string_view, std::int8_t> const modes{
//         { "",   0 },
//         { "pv", 3 }
//     };
//
//     auto it = modes.find(mode);
//     if (it == modes.end())
//     {
//         std::println("{}: lc10e::update_mode: unknown mode '{}'", name(), mode);
//         return;
//     }
//
//     if (mode_ == it->second)
//         return;
//
//     if (it->second != 0 && (state_ != &lc10e::off && state_ != &lc10e::do_off))
//         return;
//
//     mode_ = it->second;
//
//     wake_up();
// }

// void lc10e::update_speed(double value)
// {
//     speed_ = value;
//     std::println("{}: lc10e::update_speed: {}", name(), speed_);
//     wake_up();
// }

// void lc10e::switch_state(void(lc10e::*new_state)())
// {
//     state_ = new_state;
//     wake_up();
// }

// void lc10e::wake_up()
// {
//     if (deffered_call_in_proc_)
//         return;
//     deffered_call_in_proc_ = true;
//
//     eng::timer::deffered_call([this]
//     {
//         deffered_call_in_proc_ = false;
//         (this->*state_)();
//         std::fflush(stdout); 
//     });
// }

// void lc10e::off()
// {
//     std::println("{}: lc10e::off", name());
//
//     if (mode_)
//         switch_state(&lc10e::do_enable);
// }

// void lc10e::do_enable()
// {
//     std::println("{}: lc10e::do_enable", name());
//
//     // Задаем драйверу режим
//     working_mode_.set(mode_);
//
//     // Идем выполнять включение
//     switch_state(&lc10e::servo_has_no_faults);
// }

// void lc10e::do_off()
// {
//     std::println("{}: lc10e::do_off", name());
//
//     if (mode_ != 0)
//     {
//         switch_state(&lc10e::do_enable);
//         return;
//     }
//
//     switch(status_word_.get())
//     {
//     case 0x0250:
//         switch_state(&lc10e::off);
//         return;
//     case 0x0231:
//         control_word_.set(0x0000);
//         break;
//     case 0x0233:
//         control_word_.set(0x0006);
//         break;
//     case 0x0237:
//         control_word_.set(0x0007);
//         break;
//     case 0x021F:
//         switch_state(&lc10e::fault_shutdown);
//         return;
//     }
// }

// control_word_.set(0x0000); // -> 0x0250
// control_word_.set(0x0006); // -> 0x0231
// control_word_.set(0x0007); // -> 0x0233
// control_word_.set(0x000F); // -> 0x0237

// void lc10e::servo_has_no_faults()
// {
//     std::println("{}: lc10e::servo_has_no_faults", name());
//
//     if (mode_ == 0)
//     {
//         switch_state(&lc10e::do_off);
//         return;
//     }
//
//     uint16_t sw = status_word_.get();
//     // Сбрасываем флаги: "internal limit active" и "warning"
//     sw &= 0xF77F;
//
//     switch(sw)
//     {
//     case 0x0000:
//         std::println("{}: lc10e::servo_has_no_faults: 0000", name());
//         break;
//     case 0x0250:
//         std::println("{}: lc10e::servo_has_no_faults: 0250", name());
//         control_word_.set(0x0006);
//         break;
//     case 0x0231:
//         std::println("{}: lc10e::servo_has_no_faults: 0231", name());
//         control_word_.set(0x0007);
//         break;
//     case 0x0233:
//         std::println("{}: lc10e::servo_has_no_faults: 0233", name());
//         control_word_.set(0x000F);
//         break;
//     case 0x0237:
//         std::println("{}: lc10e::servo_has_no_faults: 0237", name());
//         switch_state(&lc10e::servo_running);
//         return;
//     case 0x0637:
//         std::println("{}: lc10e::servo_has_no_faults: 0637", name());
//         switch_state(&lc10e::servo_running);
//         return;
//     case 0x021F:
//         std::println("{}: lc10e::servo_has_no_faults: 021F", name());
//         switch_state(&lc10e::fault_shutdown);
//         return;
//     default:
//         std::println("{}: lc10e::servo_has_no_faults: XXXX({})", name(), status_word_.get());
//     }
// }

// void lc10e::fault_shutdown()
// {
//     std::println("{}: lc10e::fault_shutdown", name());
// }

// void lc10e::servo_running()
// {
//     std::println("{}: lc10e::servo_running: {}", name(), speed_);
//     target_speed_.set(speed_);
// }

// Находимся в режиме удержания скорости
// Для работы в данном режиме нам необходимо выставить:
// Acceleration:    0x6083:00 (u32) ускорение
// Constant Speed:  0x60ff:00 (s32) целевую скорость
// Deceleration:    0x6084:00 (u32) замедление
// Working Mode:    0x6060:00 (s8)  режим работы 3
// Current Mode:    0x6061:00 (s8)  режим работы, должен быть равен 3
// void lc10e::profile_speed_mode()
// {
    // Если режим больше не равен 3, мы должные выставить 0 и завершить работу
    // if (current_mode


    // Состоит из этапов
    // -> Acceleration
    // -> Constant Speed
    // -> Deceleration
    // Убеждаемся что мы в режиме 3
    // Обновляем значение текущей скорости
// }

