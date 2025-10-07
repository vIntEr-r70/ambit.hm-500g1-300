#include "panel-ctrl.h"

#include <hardware.h>

panel_ctrl::panel_ctrl(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("panel-ctrl")
    , base_ctrl(name(), hw, axis_cfg)
    , cnc_(hw.get_unit_by_class("", "cnc"))
    , rcu_led_(hw.get_unit_by_class("panel-ctrl", "led"), "panel-ctrl")
{ 
    for (auto axis : we::axis_cfg::LIST)
    {
        bit_state_[axis] = { false, false };
        
        hw.LSET.add(name(), fmt::format("m-{}", axis), [axis, this](nlohmann::json const &value) {
            independent_move(axis, 0, value.get<bool>());
        });
        hw.LSET.add(name(), fmt::format("p-{}", axis), [axis, this](nlohmann::json const &value) {
            independent_move(axis, 1, value.get<bool>());
        });
    }
}

void panel_ctrl::independent_move(char axis, int direction, bool state)
{
    auto &bit_state = bit_state_[axis];
    if (bit_state[direction] == state)
        return;
    bit_state[direction] = state;
    
    auto &tsp_state = tsp_state_[axis];
    if (bit_state[0] == bit_state[1])
        tsp_state = 0;
    else
        tsp_state = bit_state[0] ? -1 : 1;
    
    if (tsp_state == 0)
        hw_.log_message(LogMsg::Command, name(), fmt::format("Останов независимого движения по оси {}", axis));
    else
        hw_.log_message(LogMsg::Command, name(), fmt::format("Независимое движение по оси {} в направлении {}", axis, tsp_state));
}

void panel_ctrl::sync_state() noexcept
{
    // Текущее состояние тумблеров пульта делаем своим актуальным 
    // и не реагируем на него никак, чтобы не было сюрпризов 
    // в момент активации режима
    in_proc_ = tsp_state_;
}

void panel_ctrl::on_activate() noexcept
{
    // Синхронизируем конфигурацию
    update_internal_axis_cfg();
    next_state(&panel_ctrl::turn_off_led);
}

void panel_ctrl::on_deactivate() noexcept
{
    next_state(&panel_ctrl::stop_moving);
}

void panel_ctrl::stop_moving() noexcept
{
    while (!in_proc_.empty())
    {
        auto pair = *in_proc_.begin();
        in_proc_.erase(in_proc_.begin());

        if (pair.second == 0)
            continue;

        char axis = pair.first;
        axis_ = { axis, 0 };
        
        char paxis = axis_cfg_.axis(axis).pair_axis;
        if (paxis != '\0')
        {
            pair_axis_ = {{ axis, 0 }, { paxis, 0 }};
            next_state(&panel_ctrl::pair_stop_independent_move);
            return;
        }

        next_state(&panel_ctrl::stop_independent_move);
        return;
    }

    next_state(nullptr);
}

void panel_ctrl::turn_off_led() noexcept
{
    auto [ done, error ] = rcu_led_.set(true);
    if (!dhresult::check(done, error, [this] { next_state(&panel_ctrl::ctrl_allow); }))
        return;
    next_state(&panel_ctrl::ctrl_allow);
}

// Штатная работа режима
void panel_ctrl::ctrl_allow() noexcept
{
    check_new_axis_cfg();

    // Перебираем все оси, которыми мы можем управлять и синхронизируем состояния
    for (auto &task : tsp_state_)
    {
        char const axis = task.first;
        
        // Так как нам могут накидать любых осей
        // Убеждаемся что данная ось активна
        if (!axis_cfg_.axis(axis).use())
            continue;

        // Внутренее положение тумблера
        int idir = in_proc_[axis];
        // Внешнее положение тумблера
        int edir = task.second;

        // Если они совпадают, ничего не делаем
        if (idir == edir)
            continue;

        float const speed = speed_[axis];
        axis_ = { axis, edir };

        char paxis = axis_cfg_.axis(axis).pair_axis;
        if (paxis != '\0')
        {
            if (axis < paxis)
                pair_axis_ = {{ axis, edir * speed }, { paxis, edir * speed * -1 }};
            else 
                pair_axis_ = {{ axis, edir * speed }, { paxis, edir * speed }};

            next_state(&panel_ctrl::pair_init_moving);
            return;
        }

        move_speed_ = edir * speed;

        next_state(&panel_ctrl::init_moving);
        return;
    }
}

void panel_ctrl::init_moving() noexcept
{
    // Проверяем, в каком состоянии ось
    if (cnc_.is_in_independent_mode(axis_.first))
    {
        next_state(&panel_ctrl::independent_move);
        return;
    }
    // Иначе переводим ось в независимый режим
    next_state(&panel_ctrl::switch_to_independent_mode);
}

void panel_ctrl::pair_init_moving() noexcept
{
    char axis = pair_axis_.begin()->first;
    char paxis = std::next(pair_axis_.begin())->first;
    
    // Проверяем, в каком состоянии ось
    if (cnc_.is_in_independent_mode(axis) && cnc_.is_in_independent_mode(paxis))
    {
        next_state(&panel_ctrl::pair_independent_move);
        return;
    }
    
    // Иначе переводим ось в независимый режим
    next_state(&panel_ctrl::pair_switch_to_independent_mode);
}

void panel_ctrl::switch_to_independent_mode() noexcept
{
    auto [ done, error ] = cnc_.switch_to_independent_mode(axis_.first);
    if (!dhresult::check(done, error, [this] { next_state(&panel_ctrl::independent_error); }))
        return;
    // Переключились
    next_state(&panel_ctrl::independent_move);
}

void panel_ctrl::pair_switch_to_independent_mode() noexcept
{
    char axis = pair_axis_.begin()->first;
    char paxis = std::next(pair_axis_.begin())->first;
    
    auto [ done, error ] = cnc_.switch_to_independent_mode({ axis, paxis });
    if (!dhresult::check(done, error, [this] { next_state(&panel_ctrl::independent_error); }))
        return;
    // Переключились
    next_state(&panel_ctrl::pair_independent_move);
}

void panel_ctrl::independent_move() noexcept
{
    auto [ done, error ] = cnc_.independent_move(axis_.first, move_speed_);
    if (!dhresult::check(done, error, [this] { next_state(&panel_ctrl::independent_error); }))
        return;

    // Команда выполнена, обновляем состояние
    in_proc_[axis_.first] = axis_.second;

    next_state(&panel_ctrl::ctrl_allow);
}

void panel_ctrl::pair_independent_move() noexcept
{
    auto [ done, error ] = cnc_.independent_move(pair_axis_);
    if (!dhresult::check(done, error, [this] { next_state(&panel_ctrl::independent_error); }))
        return;

    // Команда выполнена, обновляем состояние
    in_proc_[axis_.first] = axis_.second;

    next_state(&panel_ctrl::ctrl_allow);
}

void panel_ctrl::stop_independent_move() noexcept
{
    auto [ done, error ] = cnc_.independent_move(axis_.first, 0);
    if (!dhresult::check(done, error, [this] { next_state(&panel_ctrl::stop_moving); }))
        return;
    next_state(&panel_ctrl::stop_moving);
}

void panel_ctrl::pair_stop_independent_move() noexcept
{
    auto [ done, error ] = cnc_.independent_move(pair_axis_);
    if (!dhresult::check(done, error, [this] { next_state(&panel_ctrl::stop_moving); }))
        return;
    next_state(&panel_ctrl::stop_moving);
}

void panel_ctrl::independent_error() noexcept
{
    char const axis = axis_.first;

    // Сбрасываем состояние
    tsp_state_[axis] = in_proc_[axis];

    hw_.log_message(LogMsg::Error, 
        fmt::format("Не удалось выполнить команду независимого движения для оси {}", axis));

    next_state(&panel_ctrl::ctrl_allow);
}

float panel_ctrl::get_speed(we::axis_desc const &ad, std::size_t tsp) const noexcept
{
    return ad.speed[tsp];
}

