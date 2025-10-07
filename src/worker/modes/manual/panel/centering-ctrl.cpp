#include "centering-ctrl.h"
#include "flags.h"

#include <hardware.h>

// Возможны 2 типа центровки
// 1 - зуб
// * движемся со скоростью x0.01 вправо до касания
// * возвращаемся обратно с нормальной скоростью
// * движемся влево со скоростью x0.01 до касания
// * рассчитываем центр, движемся в него с нормальной скоростью
// * движемся в глубь зуба со скоростью x0.01 до касания
// * возвращаемся на заданную величину
// 2 - элипс
// * движемся со скоростью x0.01 вправо до касания
// * возвращаемся обратно с нормальной скоростью
// * движемся влево со скоростью x0.01 до касания
// * рассчитываем центр, движемся в него с нормальной скоростью
// * движемся вверх со скоростью x0.01 до касания
// * возвращаемся обратно с нормальной скоростью
// * движемся вниз со скоростью x0.01 до касания
// * возвращаемся в центр или на заданную величину

centering_ctrl::centering_ctrl(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("centering-mode")
    , hw_(hw)
    , a_centering_(hw, axis_cfg)
    , cnc_(hw.get_unit_by_class("", "cnc"))
{ 
    hw.SET.add("centering", "terminate", [this](nlohmann::json const &) {
        ctype_ = centering_type::not_active;
    });
    
    hw.SET.add("centering", "shaft", [this](nlohmann::json const &) {
        ctype_ = centering_type::shaft;
    });
    
    hw.SET.add("centering", "tooth", [this](nlohmann::json const &args)
    {
        shift_ = args[0].get<float>();
        
        bool v = args[1].get<bool>();
        ctype_ = v ? centering_type::tooth_in : centering_type::tooth_out;
    });
}

void centering_ctrl::sync_state() noexcept
{
    ctype_ = centering_type::not_active;
}

void centering_ctrl::on_activate() noexcept
{
    // Ждем активации
    if (ctype_ == centering_type::not_active)
        return;

    bool result;

    switch(ctype_)
    {
    case centering_type::tooth_in:
        result = a_centering_.init_tooth(-1, shift_);
        break;
    case centering_type::tooth_out:
        result = a_centering_.init_tooth(1, shift_);
        break;

    case centering_type::shaft:
        result = a_centering_.init_shaft();
        break;

    default:
        ctype_ = centering_type::not_active;
        hw_.log_message(LogMsg::Error, "Попытка запустить неизвестный тип центровки детали");
        return;
    }

    if (!result)
    {
        ctype_ = centering_type::not_active;
        hw_.log_message(LogMsg::Error, a_centering_.emsg());
        return;
    }

    init_step();

    // Блокируем срабатывание глобального контроля касания 
    hw_.set_flag(flags::bki_lock_ignore, true);
    aem::log::error("-----------------centering_ctrl::on_activate: SET bki_lock_ignore");


    next_state(&centering_ctrl::do_search_steps);
}

void centering_ctrl::on_deactivate() noexcept
{
    aem::log::trace("centering_ctrl::on_deactivate 0");

    step_error();

    aem::log::trace("centering_ctrl::on_deactivate 1");

    handle_error();
}

void centering_ctrl::handle_error() noexcept
{
    if (a_centering_.proc_axis() == '\0')
    {
        aem::log::trace("centering_ctrl::on_deactivate 2");

        next_state(nullptr);
        return;
    }

    aem::log::trace("centering_ctrl::on_deactivate 3");

    if (cnc_.is_in_independent_mode(a_centering_.proc_axis()))
    {
        aem::log::trace("centering_ctrl::on_deactivate 3.1");
        next_state(&centering_ctrl::terminate_independent_move);
    }
    else
    {
        aem::log::trace("centering_ctrl::on_deactivate 3.2");
        next_state(&centering_ctrl::terminate_target_move);
    }

    aem::log::trace("centering_ctrl::on_deactivate 4");
}

void centering_ctrl::on_deactivated() noexcept
{
    // Активируем срабатывание глобального контроля касания 
    hw_.set_flag(flags::bki_lock_ignore, false);
    aem::log::error("-----------------centering_ctrl::on_deactivated: RESET bki_lock_ignore");
}

void centering_ctrl::terminate_independent_move() noexcept
{
    auto [ done, error ] = cnc_.independent_move(a_centering_.proc_axis(), 0);
    if (!dhresult::check(done, error, [this] { next_state(&centering_ctrl::centering_error); }))
        return;
    next_state(nullptr);
}

void centering_ctrl::terminate_target_move() noexcept
{
    auto [ done, error ] = cnc_.stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&centering_ctrl::centering_error); }))
        return;
    next_state(&centering_ctrl::wait_terminate_target_move_stop);
}

void centering_ctrl::wait_terminate_target_move_stop() noexcept
{
    if (cnc_.status() == engine::cnc_status::idle)
    {
        next_state(nullptr);
        return;
    }

    if (cnc_.status() != engine::cnc_status::queue)
        return;

    next_state(&centering_ctrl::reset_queue);
}

void centering_ctrl::reset_queue() noexcept
{
    auto [ done, error ] = cnc_.hard_stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&centering_ctrl::centering_error); }))
        return;
    next_state(&centering_ctrl::wait_terminate_target_move_stop);
}

void centering_ctrl::do_search_steps() noexcept
{
    if (!a_centering_.touch(&a_centering::limit_search_done) && !a_centering_.is_error())
        return;

    if (a_centering_.is_error())
    {
        hw_.log_message(LogMsg::Error, a_centering_.emsg());
        next_state(&centering_ctrl::centering_error);
        return;
    }

    if (a_centering_.is_done())
    {
        next_state(&centering_ctrl::centering_done);
        return;
    }

    next_step();

    next_state(&centering_ctrl::do_move_steps);
}

void centering_ctrl::do_move_steps() noexcept
{
    if (!a_centering_.touch(&a_centering::move_to_point_done) && !a_centering_.is_error())
        return;

    if (a_centering_.is_error())
    {
        hw_.log_message(LogMsg::Error, a_centering_.emsg());
        next_state(&centering_ctrl::centering_error);
        return;
    }

    next_step();

    next_state(&centering_ctrl::do_search_steps);
}

void centering_ctrl::centering_done() noexcept
{
    hw_.log_message(LogMsg::Info, "Центровка детали завершена");
    hw_.notify({ "sys", "centering-step" }, 0);

    ctype_ = centering_type::not_active;
    next_state(nullptr);
}

void centering_ctrl::centering_error() noexcept
{
    step_error();
    hw_.log_message(LogMsg::Error, "Не удалось выполнить поиск центра");

    ctype_ = centering_type::not_active;

    handle_error();
}

void centering_ctrl::init_step() noexcept
{
    step_ = 1;
    hw_.notify({ "sys", "centering-step" }, 1);
}

void centering_ctrl::next_step() noexcept
{
    step_ += 1;
    hw_.notify({ "sys", "centering-step" }, step_);
}

void centering_ctrl::step_error() noexcept
{
    aem::log::error("centering_step_error: {}", step_);
    hw_.notify({ "sys", "centering-step" }, -static_cast<int>(step_));
}
