#include "move-to-ctrl.h"

#include <hardware.h>

move_to_ctrl::move_to_ctrl(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("move-to-ctrl")
    , base_ctrl(name(), hw, axis_cfg)
    , cnc_(hw.get_unit_by_class("", "cnc"))
{ 
    hw.SET.add("cnc", "axis-move-to", [this](nlohmann::json const &args)
    {
        char axis = args[0].get<char>();
        float pos = args[1].get<float>();
        set_move_to(axis, pos);
    });
}

void move_to_ctrl::sync_state() noexcept
{
    // Очищаем все, что могли нащелакать пока мы были не активны
    move_to_.first = 0;
}

void move_to_ctrl::set_move_to(char axis, float pos) noexcept
{
    move_to_ = { axis, pos };
}

void move_to_ctrl::on_activate() noexcept
{
    // Синхронизируем конфигурацию
    update_internal_axis_cfg();

    next_state(&move_to_ctrl::ctrl_allow);
}

void move_to_ctrl::on_deactivate() noexcept
{
    next_state(&move_to_ctrl::stop_target_move);
}

// Штатная работа режима
void move_to_ctrl::ctrl_allow() noexcept
{
    check_new_axis_cfg();

    // Проверяем необходимость движения в конкретную позицию
    if (move_to_.first == 0)
        return;

    real_move_to_ = move_to_;
    move_to_.first = 0;

    // Останавливаем текущее движение

    char const axis = real_move_to_.first;
    real_speed_ = speed_[axis];

    // Проверяем, в каком состоянии ось
    if (!cnc_.is_in_independent_mode(axis))
    {
        next_state(&move_to_ctrl::target_move);
        return;
    }

    // Иначе переводим ось в зависимый режим
    next_state(&move_to_ctrl::switch_to_target_mode);
}

void move_to_ctrl::switch_to_target_mode() noexcept
{
    auto [ done, error ] = cnc_.switch_to_target_mode(real_move_to_.first);
    if (!dhresult::check(done, error, [this] { next_state(&move_to_ctrl::target_error); }))
        return;
    // Переключились
    next_state(&move_to_ctrl::target_move);
}

void move_to_ctrl::target_move() noexcept
{
    auto [ done, error ] = cnc_.target_move(real_move_to_.first, real_move_to_.second, real_speed_);
    if (!dhresult::check(done, error, [this] { next_state(&move_to_ctrl::target_error); }))
        return;
    next_state(&move_to_ctrl::wait_target_move_stop);
}

void move_to_ctrl::stop_target_move() noexcept
{
    auto [ done, error ] = cnc_.stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&move_to_ctrl::target_stop_error); }))
        return;
    next_state(&move_to_ctrl::wait_target_move_stop);
}

void move_to_ctrl::wait_target_move_stop() noexcept
{
    if (cnc_.status() == engine::cnc_status::idle)
    {
        next_state(nullptr);
        return;
    }

    if (cnc_.status() != engine::cnc_status::queue)
        return;

    next_state(&move_to_ctrl::reset_queue);
}

void move_to_ctrl::reset_queue() noexcept
{
    auto [ done, error ] = cnc_.hard_stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&move_to_ctrl::target_stop_error); }))
        return;
    next_state(&move_to_ctrl::wait_target_move_stop);
}

void move_to_ctrl::target_error() noexcept
{
    hw_.log_message(LogMsg::Error, 
        fmt::format("Не удалось выполнить команду движения в позицию для оси {}", real_move_to_.first));
    next_state(nullptr);
}
    
void move_to_ctrl::target_stop_error() noexcept
{
    hw_.log_message(LogMsg::Error, "Не удалось выполнить команду остановки движения");
    next_state(nullptr);
}

float move_to_ctrl::get_speed(we::axis_desc const &ad, std::size_t tsp) const noexcept
{
    return ad.speed[tsp];
}

