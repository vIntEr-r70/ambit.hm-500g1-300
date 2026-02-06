#include "axis-motion-node.hpp"
#include "slaves/servo-motor.hpp"

#include "common/axis-config.hpp"

#include <eng/timer.hpp>
#include <eng/log.hpp>
#include <eng/sibus/client.hpp>

axis_motion_node::axis_motion_node(char axis, servo_motor &servo_motor)
    : eng::sibus::node(std::format("axis-motion-{}", axis)) 
    , axis_(axis)
    , servo_motor_(servo_motor)
    , by_position_(*this)
    , by_velocity_(*this)
    , by_time_(*this)
{
    // Входящий провод для сигналов управления
    ictl_ = node::add_input_wire();

    // Выходной порт с состоянием
    position_out_ = node::add_output_port("position");
    speed_out_ = node::add_output_port("speed");

    std::string key = std::format("axis/{}", axis);
    eng::sibus::client::config_listener(key, [this](std::string_view json)
    {
        if (json.empty()) return;

        axis_config::desc desc;
        axis_config::load(desc, json);

        eng::log::info("{}: acc = {}, ratio = {}", name(), desc.acc, desc.ratio);

        servo_motor_.set_acc_limit(desc.acc);
        servo_motor_.set_ratio(desc.ratio);
    });

    node::set_activate_handler(ictl_, [this](eng::abc::pack args)
    {
        start_command_execution(std::move(args));
    });

    node::set_deactivate_handler(ictl_, [this]
    {
        deactivate();
    });

    auto tid = eng::timer::create([this] {
        update_output_info();
    });
    eng::timer::start(tid, std::chrono::milliseconds(300));
}

// Провод от нас отключен
void axis_motion_node::deactivate()
{
    eng::log::info("axis_motion_node[{}]::node_offline", axis_);

    // Если ресурс так и не получен либо не выполняет движений
    // нет необходимости его финализировать, просто освобождаем
    if (servo_motor_.control() == nullptr)
    {
        node::set_ready(ictl_);
        return;
    }

    eng::log::info("axis_motion_node[{}]::node_offline: WAIT FOR DONE", axis_);

    // Необходимо сначала привести драйвер в исходное состояние
    // Остановить выполнение движения если оно происходит
    // Освобождаем ресурс чтобы другие могли воспользоваться
    by_position_.update_speed_limit(0.0);
    by_velocity_.update_speed_limit(0.0);
    by_time_.reset();
}

double axis_motion_node::local_position() const noexcept
{
    return servo_motor_.position() - origin_offset_;
}

void axis_motion_node::update_output_info()
{
    node::set_port_value(position_out_, { local_position() });
    node::set_port_value(speed_out_, { servo_motor_.speed() });
}

bool axis_motion_node::motion_done()
{
    update_output_info();

    eng::log::info("axis_motion_node[{}]::motion_done", axis_);
    node::set_ready(ictl_);

    return false;
}

void axis_motion_node::motion_status()
{
}

void axis_motion_node::start_command_execution(eng::abc::pack args)
{
    eng::log::info("{}: start_command_execution: args-size: {}", name(), args.size());

    static std::unordered_map<std::string_view, commands_handler> const map {
        { "move-to",        &axis_motion_node::cmd_move_to          },
        { "shift",          &axis_motion_node::cmd_shift            },
        { "spin",           &axis_motion_node::cmd_spin             },
        { "stop",           &axis_motion_node::cmd_stop             },
        { "zerro",          &axis_motion_node::cmd_zerro            },
        { "timed-shift",    &axis_motion_node::cmd_timed_shift      }
    };

    std::string_view cmd = eng::abc::get<std::string_view>(args, 0);
    auto it = map.find(cmd);
    if (it == map.end())
    {
        node::terminate(ictl_, std::format("Незнакомая комманда: {}", cmd));
        return;
    }

    args.pop_front();

    if ((this->*(it->second))(args))
        return;

    node::terminate(ictl_, "Недопустимый режим движения");
}

bool axis_motion_node::cmd_move_to(eng::abc::pack const &args)
{
    double pos = eng::abc::get<double>(args, 0);
    double speed = eng::abc::get<double>(args, 1);

    motion_control *mc = servo_motor_.control();
    if (mc != nullptr && mc != &by_position_)
        return false;

#ifndef BUILDROOT
    speed = 10.0;
#endif

    by_position_.update_speed_limit(speed);
    by_position_.update_absolute_target(pos + origin_offset_);

    if (mc == nullptr)
        servo_motor_.set_control(by_position_);

    return true;
}

bool axis_motion_node::cmd_shift(eng::abc::pack const &args)
{
    double dpos = eng::abc::get<double>(args, 0);
    double speed = eng::abc::get<double>(args, 1);

    motion_control *mc = servo_motor_.control();
    if (mc != nullptr && mc == &by_position_)
        return false;

    by_position_.update_speed_limit(speed);
    by_position_.update_relative_target(dpos);

    if (mc == nullptr)
        servo_motor_.set_control(by_position_);

    return true;
}

bool axis_motion_node::cmd_spin(eng::abc::pack const &args)
{
    double speed = eng::abc::get<double>(args);

    motion_control *mc = servo_motor_.control();
    if (mc != nullptr && mc != &by_velocity_)
        return false;

    by_velocity_.update_speed_limit(speed);
    if (mc == nullptr)
        servo_motor_.set_control(by_velocity_);

    return true;
}

bool axis_motion_node::cmd_stop(eng::abc::pack const &)
{
    motion_control *mc = servo_motor_.control();

    if (mc == nullptr)
    {
        node::wire_response(ictl_, true, { });
        return true;
    }

    if (mc != &by_position_ && mc != &by_velocity_)
        return false;

    by_position_.update_speed_limit(0.0);
    by_velocity_.update_speed_limit(0.0);

    return true;
}

bool axis_motion_node::cmd_zerro(eng::abc::pack const &)
{
    // Ось находится в движении
    if (servo_motor_.control())
        return true;

    origin_offset_ = servo_motor_.position();

    update_output_info();

    node::set_ready(ictl_);

    return true;
}

// t0, N, [a, t1, t2, t3, v, v1, dS], ...
bool axis_motion_node::cmd_timed_shift(eng::abc::pack const &args)
{
    motion_control *mc = servo_motor_.control();
    if (mc != nullptr) return false;

    double t0 = eng::abc::get<double>(args, 0);
    by_time_.init(t0);

    std::uint8_t N = eng::abc::get<std::uint8_t>(args, 1);

    std::size_t iarg = 2;
    for (std::size_t i = 0; i < N; ++i)
    {
        by_time_.add_interval({
            .a  = eng::abc::get<double>(args, iarg++),
            .t1 = eng::abc::get<double>(args, iarg++),
            .t2 = eng::abc::get<double>(args, iarg++),
            .t3 = eng::abc::get<double>(args, iarg++),
            .v  = eng::abc::get<double>(args, iarg++),
            .v1 = eng::abc::get<double>(args, iarg++),
            .ds = eng::abc::get<double>(args, iarg++)
        });
    }

    servo_motor_.set_control(by_time_);

    return true;
}

void axis_motion_node::register_on_bus_done()
{
    node::set_ready(ictl_);
}

