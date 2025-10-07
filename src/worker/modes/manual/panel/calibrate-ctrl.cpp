#include "calibrate-ctrl.h"

#include <hardware.h>
#include <axis-cfg.h>

calibrate_ctrl::calibrate_ctrl(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("calibrate-mode")
    , hw_(hw)
    , axis_cfg_(axis_cfg)
    , cnc_(hw.get_unit_by_class("", "cnc"))
{ 
    hw.SET.add("cnc", "axis-calibrate", [this](nlohmann::json const &args) {
        axis_ = args[0].get<char>();
    });
}

void calibrate_ctrl::sync_state() noexcept
{
    axis_ = '\0';
}

void calibrate_ctrl::on_activate() noexcept
{
    if (axis_ == '\0')
        return;

    // Проверяем доступность калибровки для данной оси
    std::size_t axis_id = we::axis_cfg::ID(axis_);
    if (axis_id >= 6)
    {
        hw_.log_message(LogMsg::Error, fmt::format("Ось {} не имеет концевиков", axis_));
        axis_ = '\0';
        return;
    }
    else
    {
        auto const& cfg = axis_cfg_.axis(axis_);
        if (cfg.muGrads)
        {
            hw_.log_message(LogMsg::Error, fmt::format("Ось {} сконфигурирована для вращения", axis_));
            axis_ = '\0';
            return;
        }
    }

    // Запоминаем ее для дальнейшего использования
    real_axis_ = axis_;
    update_calibrate_step(0, true);

    hw_.nf_.notify({ "sys", "calibrate" }, axis_);

    limit_search_speed_ = get_limit_search_speed(1);
    on_limit_found_action_ = &calibrate_ctrl::switch_to_target_mode;

    // Проверяем, в каком состоянии ось
    if (!cnc_.is_in_independent_mode(real_axis_))
    {
        next_state(&calibrate_ctrl::switch_to_independent_mode);
        return;
    }

    next_state(&calibrate_ctrl::limit_search_move);
}

void calibrate_ctrl::on_deactivate() noexcept
{
    if (real_axis_ == '\0')
    {
        next_state(nullptr);
        return;
    }

    calibrate_step_error();

    if (cnc_.is_in_independent_mode(real_axis_))
        next_state(&calibrate_ctrl::terminate_independent_move);
    else
        next_state(&calibrate_ctrl::terminate_target_move);
}

void calibrate_ctrl::terminate_independent_move() noexcept
{
    auto [ done, error ] = cnc_.independent_move(real_axis_, 0);
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;

    real_axis_ = '\0';
    hw_.nf_.notify({ "sys", "calibrate" }, '\0');

    next_state(nullptr);
}

void calibrate_ctrl::terminate_target_move() noexcept
{
    auto [ done, error ] = cnc_.stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;
    next_state(&calibrate_ctrl::wait_terminate_target_move_stop);
}

void calibrate_ctrl::wait_terminate_target_move_stop() noexcept
{
    if (cnc_.status() == engine::cnc_status::idle)
    {
        real_axis_ = '\0';
        hw_.nf_.notify({ "sys", "calibrate" }, '\0');

        next_state(nullptr);
        return;
    }

    if (cnc_.status() != engine::cnc_status::queue)
        return;

    next_state(&calibrate_ctrl::reset_queue);
}

void calibrate_ctrl::reset_queue() noexcept
{
    auto [ done, error ] = cnc_.hard_stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;
    next_state(&calibrate_ctrl::wait_terminate_target_move_stop);
}

void calibrate_ctrl::switch_to_independent_mode() noexcept
{
    auto [ done, error ] = cnc_.switch_to_independent_mode(real_axis_);
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;
    // Переключились, инициируем движение
    next_state(&calibrate_ctrl::limit_search_move);
}

void calibrate_ctrl::limit_search_move() noexcept
{
    auto [ done, error ] = cnc_.independent_move(real_axis_, limit_search_speed_);
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;
    // Драйвер должен был начать движение, ждем сигнала концевика
    next_state(&calibrate_ctrl::wait_limit_search_done);
}

void calibrate_ctrl::wait_limit_search_done() noexcept
{
    // Ждем пока произойдет детектирование
    // Ждем завершения движения
    if (!cnc_.is_limit(real_axis_, (limit_search_speed_ < 0) ? -1 : 1))
        return;
    // Концевик найден, выполняем соответствующее действие
    next_state(on_limit_found_action_);
}

void calibrate_ctrl::switch_to_target_mode() noexcept
{
    auto [ done, error ] = cnc_.switch_to_target_mode(real_axis_);
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;

    update_calibrate_step(1);

    // Переключились, съезжаем с концевика
    next_state(&calibrate_ctrl::move_out_from_limit);
}

// Задача съехать с концевика
// Для этого шагаем назад по 1мм и проверяем, пропал или нет
void calibrate_ctrl::move_out_from_limit() noexcept
{
    // Настройки оси
    auto const& cfg = axis_cfg_.axis(real_axis_);
    float speed = cfg.speed[0]; 

    auto [ done, error ] = cnc_.relative_target_move(real_axis_, cfg.home ? -1 : 1, speed);
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;

    // Команда принята, переходим на ожидание выполнения
    next_state(&calibrate_ctrl::wait_move_out_done);
}

void calibrate_ctrl::wait_move_out_done() noexcept
{
    if (cnc_.status() == engine::cnc_status::run)
        return;

    // Проверяем срабатывание концевика
    if (cnc_.is_limit(real_axis_, (limit_search_speed_ < 0) ? -1 : 1))
    {
        // Он все еще установлен, повторяем предидущий шаг
        next_state(&calibrate_ctrl::move_out_from_limit);
        return;
    }

    // Сигнал концевика пропал, отъезжаем еще на 10мм
    next_state(&calibrate_ctrl::move_away_from_limit);
}

void calibrate_ctrl::move_away_from_limit() noexcept
{
    // Настройки оси
    auto const& cfg = axis_cfg_.axis(real_axis_);
    float speed = cfg.speed[0]; 

    auto [ done, error ] = cnc_.relative_target_move(real_axis_, cfg.home ? -10 : 10, speed);
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;

    // Команда принята, переходим на ожидание выполнения
    next_state(&calibrate_ctrl::wait_move_away_done);
}

void calibrate_ctrl::wait_move_away_done() noexcept
{
    // Ожидаем завершения движения
    if (cnc_.status() == engine::cnc_status::run)
        return;

    update_calibrate_step(2);

    // Инициируем повторный поиск концевика с меньшей скоростью
    limit_search_speed_ = get_limit_search_speed(0);

    // По завершении идем на завершение процесса калибровки
    on_limit_found_action_ = &calibrate_ctrl::calibrate_done;

    // Повторяем движение к концевику с меньшей скоростью
    next_state(&calibrate_ctrl::switch_to_independent_mode);
}

void calibrate_ctrl::calibrate_done() noexcept
{
    update_calibrate_step(3);

    // Настройки оси
    auto const& cfg = axis_cfg_.axis(real_axis_);

    auto [ done, error ] = cnc_.change_driver_axis_position(real_axis_, cfg.home ? cfg.length : 0);
    if (!dhresult::check(done, error, [this] { next_state(&calibrate_ctrl::calibrate_error); }))
        return;

    hw_.notify({ "sys", "calibrate" }, '\0');
    hw_.notify({ "sys", "calibrate-step" }, 0);

    // Калибровка завершена, мы дома
    hw_.log_message(LogMsg::Info, fmt::format("Калибровка оси {} завершена", real_axis_));

    real_axis_ = '\0';
    axis_ = '\0';

    next_state(nullptr);
}

void calibrate_ctrl::calibrate_error() noexcept
{
    calibrate_step_error();

    hw_.nf_.notify({ "sys", "calibrate" }, '\0');

    hw_.log_message(LogMsg::Error, 
        fmt::format("Не удалось выполнить калибровку оси {}", real_axis_));

    real_axis_ = '\0';
    axis_ = '\0';

    next_state(nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////

void calibrate_ctrl::update_calibrate_step(std::size_t step, bool force) noexcept
{
    step += 1;

    if (step_ == step && !force)
        return;
    step_ = step;

    aem::log::error("update_calibrate_step: {}", step);
    hw_.nf_.notify({ "sys", "calibrate-step" }, step);
}

void calibrate_ctrl::calibrate_step_error() noexcept
{
    aem::log::error("calibrate_step_error: {}", step_);
    hw_.nf_.notify({ "sys", "calibrate-step" }, -static_cast<int>(step_));
}

float calibrate_ctrl::get_limit_search_speed(std::size_t id) const noexcept
{
    // Настройки калибруемой оси
    auto const& cfg = axis_cfg_.axis(real_axis_);
    // Скорость поиска концевика, если дом MIN(false) едем в минус 
    return cfg.speed[id] * (cfg.home ? 1 : -1);
}


