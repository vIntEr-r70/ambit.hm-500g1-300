#include "rcu-ctl.hpp"
#include "common/load-axis-list.hpp"
#include "common/axis-config.hpp"

#include <eng/sibus/client.hpp>

#include <numeric>

// Просто собираем цепь на основании значения входного контакта
// Связь всегда один к одному либо отсутствует

rcu_ctl::rcu_ctl()
    : eng::sibus::node("rcu-ctl")
{
    // Реакция на входной сигнал от пультика
    for (auto key : { 'X', 'Y', 'Z', '4', '5', '6' })
    {
        add_input_port(std::string(1, key), [this, key](eng::abc::pack args)
        {
            tsp_select_map_[key] = eng::abc::get<bool>(args);
            update_axis_selection();
        });
    }

    ambit::load_axis_list([this](char axis, std::string_view, bool)
    {
        axis_map_[axis].ctl = node::add_output_wire(std::string(1, axis));
        node::set_wire_status_handler(axis_map_[axis].ctl, [this, axis]
        {
        });

        axis_map_[axis].linked_tsp_key = '\0';
        axis_map_[axis].speed = { 0.0, 0.0, 0.0 };
        axis_map_[axis].ratio = 0.0;

        std::string key = std::format("axis/{}", axis);
        eng::sibus::client::config_listener(key, [this, axis](std::string_view json)
        {
            if (json.empty()) return;

            axis_config::desc desc;
            axis_config::load(desc, json);

            axis_map_[axis].linked_tsp_key = desc.rcu.link_axis;
            axis_map_[axis].speed = desc.rcu.speed;
            axis_map_[axis].ratio = desc.rcu.ratio;
        });
    });

    add_input_port("ss0", [this](eng::abc::pack value) {
        speed_select_.set(0, eng::abc::get<bool>(value));
    });

    add_input_port("ss1", [this](eng::abc::pack value) {
        speed_select_.set(1, eng::abc::get<bool>(value));
    });

    add_input_port_unsafe("spin", [this](eng::abc::pack args) {
        spin_value_changed(std::move(args));
    });

    led_ = add_output_port("led");

    tid_ = eng::timer::create([this]
    {
        add_next_value(axis_, 0.0);
    });
    filter_.resize(10, 0.0);
}

// Каждый раз как приходит сигнал, определяем, какой осью мы управляем
void rcu_ctl::update_axis_selection()
{
    // Получаем активный tsp
    char tsp_key = get_selected_tsp();

    // Ищем в конфигурациях, к какой оси он привязана
    char prev_axis = axis_;
    axis_ = tsp_key ? get_selected_axis(tsp_key) : '\0';

    node::set_port_value(led_, { axis_ ? false : true });

    // Тангету отпустили или не обрабатываемый tsp
    if (axis_ == '\0' && prev_axis != '\0')
    {
        reset_filter(prev_axis);

        // auto const &axis = axis_map_[prev_axis];
        // node::activate(axis.ctl, { "stop" });

        return;
    }
}

char rcu_ctl::get_selected_tsp() const noexcept
{
    // Ищем первый активный
    auto it = std::ranges::find_if(tsp_select_map_, [](auto const &pair) {
        return pair.second;
    });
    // Возвращаем то что нашли
    return (it == tsp_select_map_.end()) ? '\0' : it->first;
}

char rcu_ctl::get_selected_axis(char tsp_key) const noexcept
{
    // Ищем значение в конфигурации
    auto it = std::ranges::find_if(axis_map_, [tsp_key](auto const &pair) {
        return pair.second.linked_tsp_key == tsp_key;
    });
    // Возвращаем то что нашли
    return (it == axis_map_.end()) ? '\0' : it->first;
}

void rcu_ctl::spin_value_changed(eng::abc::pack args)
{
    if (!args)
    {
        position_.reset();
        return;
    }

    std::int32_t new_position = eng::abc::get<std::int32_t>(args);

    // Инициализируемся
    if (!position_.has_value())
    {
        position_ = new_position;
        return;
    }

    auto diff = new_position - position_.value();
    position_ = new_position;

    if (axis_ != '\0')
        update_axis_position(axis_, diff);
}

void rcu_ctl::update_axis_position(char axis, double diff)
{
    auto &ref = axis_map_[axis_];

    // if (!node::is_ready(ref.ctl))
    //     return;
    //
    double shift = diff / ref.ratio;
    std::size_t idx = std::min<std::size_t>
        (speed_select_.to_ulong(), ref.speed.size() - 1);
    double speed = ref.speed[idx];

    // eng::log::info("{}: speed = {:.3f}, shift = {:.3f},", name(), speed, shift);
    //
    // node::activate(ref.ctl, { "shift", shift, speed });

    if (!eng::timer::is_running(tid_))
        eng::timer::start(tid_, std::chrono::milliseconds(50));

    add_next_value(axis, shift * speed);
}

void rcu_ctl::add_next_value(char axis, double value)
{
    filter_.pop_back();
    filter_.insert(filter_.begin(), value);
    calculate_next_value(axis);
}

void rcu_ctl::reset_filter(char axis)
{
    std::ranges::fill(filter_, 0.0);
    calculate_next_value(axis);
}

void rcu_ctl::calculate_next_value(char axis)
{
    auto sum = std::accumulate(filter_.begin(), filter_.end(), 0.0);
    double speed = sum * 2.0;

    auto &ref = axis_map_[axis];

    std::size_t idx = std::min<std::size_t>
        (speed_select_.to_ulong(), ref.speed.size() - 1);
    double limit = std::min(std::abs(speed), ref.speed[idx]);
    speed = std::copysign(limit, speed);

    if (last_speed_ != speed)
    {
        eng::log::info("{}.{}: speed = {}", name(), axis, speed);

        node::activate(ref.ctl, { "spin", speed });
        last_speed_ = speed;
    }

    if (sum == 0.0)
    {
        if (eng::timer::is_running(tid_))
            eng::timer::stop(tid_);
    }

}

