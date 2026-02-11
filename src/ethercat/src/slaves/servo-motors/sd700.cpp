#include "sd700.hpp"
#include "slaves/servo-motor.hpp"

#include <bitset>

#include "ethercat.hpp"

// #include <eng/timer.hpp>
// #include <eng/abc/lambda-traits.hpp>

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
    // ethercat::pdo::add(this, 0x60FF, 0x00, target_speed_);
    ethercat::pdo::add(this, 0x607A, 0x00, target_pos_);

    ethercat::pdo::add(this, 0x6041, 0x00, status_word_);
    ethercat::pdo::add(this, 0x603f, 0x00, error_code_);
    ethercat::pdo::add(this, 0x6064, 0x00, real_pos_);

    // node::add_input_port("mode", [this](eng::abc::pack value)
    // {
    //     update_mode(eng::abc::get<std::string_view>(value));
    // });

    // node::add_input_port("speed", [this](eng::abc::pack value)
    // {
    //     update_speed(eng::abc::get<double>(value));
    // });

    // node::add_input_port("position", [this](eng::abc::pack value)
    // {
    //     update_position(eng::abc::get<std::int32_t>(value));
    // });

    // switch_state(&sd700::off);
}

void sd700::set_target_pos(std::int32_t value)
{
    // Просто задаем текущую позицию драйверу
    target_pos_.set(value);
}

void sd700::set_target_speed(std::int32_t value)
{
    // Просто задаем текущую скорость драйверу
    target_speed_.set(value);
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

void sd700::activate_probe()
{
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

// void sd700::update_mode(std::string_view mode)
// {
//     std::println("{}: sd700::update_mode: [{}]", name(), mode);
//
//     static std::unordered_map<std::string_view, std::int8_t> const modes{
//         { "",    0 },
//         { "pv",  3 },
//         { "csp", 8 }
//     };
//
//     auto it = modes.find(mode);
//     if (it == modes.end())
//     {
//         std::println("{}: sd700::update_mode: unknown mode '{}'", name(), mode);
//         return;
//     }
//
//     if (mode_ == it->second)
//         return;
//
//     if (it->second != 0 && (state_ != &sd700::off && state_ != &sd700::do_off))
//         return;
//
//     mode_ = it->second;
//
//     wake_up();
// }
//
// void sd700::update_speed(double value)
// {
//     speed_ = value;
//     std::println("{}: sd700::update_speed: {}", name(), speed_);
//     wake_up();
// }
//
// void sd700::update_position(std::int32_t value)
// {
//     position_ = value;
//     std::println("{}: sd700::update_position: {}", name(), position_);
//     wake_up();
// }
//
// void sd700::switch_state(void(sd700::*new_state)())
// {
//     state_ = new_state;
//     wake_up();
// }
//
// void sd700::wake_up()
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
//
// void sd700::off()
// {
//     std::println("{}: sd700::off", name());
//
//     if (mode_)
//         switch_state(&sd700::do_enable);
// }
//
// void sd700::do_enable()
// {
//     std::println("{}: lc10e::do_enable", name());
//
//     // Задаем драйверу режим
//     working_mode_.set(mode_);
//
//     // Идем выполнять включение
//     switch_state(&sd700::servo_has_no_faults);
// }
//
// void sd700::do_off()
// {
//     std::println("{}: sd700::do_off", name());
//
//     if (mode_ != 0)
//     {
//         switch_state(&sd700::do_enable);
//         return;
//     }
//
//     switch(status_word_.get())
//     {
//     case 0x0250:
//         switch_state(&sd700::off);
//         std::println("{}: sd700::do_off: sw = 0x250", name());
//         return;
//     case 0x0231:
//         control_word_.set(0x0000);
//         std::println("{}: sd700::do_off: sw = 0x231, cw = 0x0000", name());
//         break;
//     case 0x0233:
//         control_word_.set(0x0006);
//         std::println("{}: sd700::do_off: sw = 0x233, cw = 0x0006", name());
//         break;
//     case 0x0237:
//     case 0x1650:
//         std::println("{}: sd700::do_off: sw = 0x237, cw = 0x0007", name());
//         control_word_.set(0x0007);
//         break;
//     case 0x021F:
//         switch_state(&sd700::fault_shutdown);
//         std::println("{}: sd700::do_off: sw = 0x21F", name());
//         return;
//     }
// }
//
// // control_word_.set(0x0000); // -> 0x0250
// // control_word_.set(0x0006); // -> 0x0231
// // control_word_.set(0x0007); // -> 0x0233
// // control_word_.set(0x000F); // -> 0x0237
//
// void sd700::servo_has_no_faults()
// {
//     std::println("{}: sd700::servo_has_no_faults", name());
//
//     if (mode_ == 0)
//     {
//         switch_state(&sd700::do_off);
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
//         std::println("{}: sd700::servo_has_no_faults: 0000", name());
//         break;
//     case 0x0250:
//         std::println("{}: sd700::servo_has_no_faults: 0250", name());
//         control_word_.set(0x0006);
//         break;
//     case 0x0231:
//     case 0x1631:
//     case 0x163B:
//         std::println("{}: sd700::servo_has_no_faults: 0231", name());
//         control_word_.set(0x0007);
//         break;
//     case 0x0233:
//     case 0x1633:
//     case 0x1650:
//         std::println("{}: sd700::servo_has_no_faults: 0233", name());
//         control_word_.set(0x000F);
//         break;
//     case 0x0237:
//     case 0x1637:
//         std::println("{}: sd700::servo_has_no_faults: 0237", name());
//         switch_state(&sd700::servo_running);
//         return;
//     case 0x0637:
//         std::println("{}: sd700::servo_has_no_faults: 0637", name());
//         switch_state(&sd700::servo_running);
//         return;
//     case 0x021F:
//         std::println("{}: sd700::servo_has_no_faults: 021F", name());
//         switch_state(&sd700::fault_shutdown);
//         return;
//     default:
//         std::println("{}: sd700::servo_has_no_faults: XXXX({})", name(), status_word_.get());
//     }
// }
//
// void sd700::fault_shutdown()
// {
//     std::println("{}: sd700::fault_shutdown", name());
// }
//
// void sd700::servo_running()
// {
//     std::print("{}: sd700::servo_running: ", name());
//
//     switch(mode_)
//     {
//     case 3:
//         std::println("in speed mode: {}", speed_);
//         target_speed_.set(speed_);
//         break;
//     case 8:
//         std::println("in position mode: {}", position_);
//         target_position_.set(position_ * 10000);
//         break;
//     }
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

