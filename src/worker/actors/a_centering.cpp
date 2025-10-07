#include "a_centering.h"
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

#define MARKER_AXIS 'C'
#define MARKER_LIMIT -1


a_centering::a_centering(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : hw_(hw)
    , cnc_(hw.get_unit_by_class("", "cnc"))
    , axis_cfg_(axis_cfg)
{ }

bool a_centering::touch() noexcept
{
    if (state_ == nullptr)
        return true;
    std::invoke(state_, this);
    return state_ == nullptr;
}

bool a_centering::touch(handler_t wstate) noexcept
{
    if (state_ == nullptr)
        return true;
    std::invoke(state_, this);
    return state_ == nullptr || state_ == wstate;
}

// dir - internal(+1) or external(-1) tooth 
bool a_centering::init_tooth(int dir, float shift) noexcept
{
    tooth_shift_ = shift;
    axis_task_ = {{'X', { dir }}, {'Y', { 1, -1 }}};
    return init();
}

bool a_centering::init_shaft() noexcept
{
    axis_task_ = {{'X', { 1, -1 }}, {'Y', { 1, -1 }}};
    return init();
}

bool a_centering::init() noexcept
{
    state_ = nullptr;
    emsg_.clear();

    proc_axis_ = '\0';

    // Проверяем активна ли защита 
    if (cnc_.is_limit(MARKER_AXIS, MARKER_LIMIT))
    {   
        emsg_ = "Необходимо сбросить защиту от касания";
        return false;
    }

    next_state(&a_centering::init_next_axis);

    return true;
}

void a_centering::init_next_axis() noexcept
{
    // Все оси и направления обработаны, завершаем выполнение
    if (axis_task_.empty())
    {
        // Сбрасываем контроллер касания
        hw_.set_flag(flags::do_reset_bki, true);
        cnc_.set_fake_limit(MARKER_AXIS, MARKER_LIMIT);
        hw_.log_message(LogMsg::Info, "centering", "Ожидание сброса БКИ");
        
        next_state(&a_centering::wait_reset_bki_and_done);
        return;
    }

    task_ = std::move(axis_task_.back());
    axis_task_.pop_back();

    if (!axis_cfg_.axis(task_.axis).use())
    {
        emsg_ = fmt::format("Попытка использовать неинициализированную ось {}", task_.axis);
        next_state(nullptr);
        return;
    }

    // Запоминаем текущую позицию чтобы можно было вернуться
    start_point_pos_ = cnc_.pos(task_.axis);

    // Сбрасываем контроллер касания
    hw_.set_flag(flags::do_reset_bki, true);
    cnc_.set_fake_limit(MARKER_AXIS, MARKER_LIMIT);
    hw_.log_message(LogMsg::Info, "centering", "Ожидание сброса БКИ");
    
    next_state(&a_centering::wait_reset_bki);
}

void a_centering::wait_reset_bki() noexcept
{
    if (hw_.get_flag(flags::do_reset_bki) || cnc_.is_limit(MARKER_AXIS, MARKER_LIMIT))
        return;
    hw_.log_message(LogMsg::Info, "centering", "БКИ сброшен");
    
    // Инициируем поиск детали
    next_state(&a_centering::switch_to_independent_mode);
}

void a_centering::wait_reset_bki_and_done() noexcept
{
    if (hw_.get_flag(flags::do_reset_bki) || cnc_.is_limit(MARKER_AXIS, MARKER_LIMIT))
        return;
    hw_.log_message(LogMsg::Info, "centering", "БКИ сброшен");
    
    // Инициируем поиск детали
    next_state(nullptr);
}

void a_centering::switch_to_independent_mode() noexcept
{
    // Пытаемся несколько раз, ось может еще быть в движении
    auto [ done, error ] = cnc_.switch_to_independent_mode(task_.axis);
    if (!dhresult::check(done, error, [this] { next_state(&a_centering::switch_to_independent_mode); }))
        return;

    // Настройки калибруемой оси
    auto const& cfg = axis_cfg_.axis(task_.axis);
    limit_search_speed_ = cfg.speed[0] * task_.dir.back() * 0.1f; 

    // Запоминаем активированную ось чтобы можно было прервать
    proc_axis_ = task_.axis;

    hw_.log_message(LogMsg::Info, "centering", fmt::format("Поиск центра по оси {}", proc_axis_));
    
    // Переключились, инициируем движение
    next_state(&a_centering::limit_search_move);
}

void a_centering::limit_search_move() noexcept
{
    auto [ done, error ] = cnc_.independent_move(task_.axis, limit_search_speed_);
    if (!dhresult::check(done, error, [this] { set_error(fmt::format("Не удалось инициировать поиск заготовки по оси {}", task_.axis)); }))
        return;
    // Драйвер должен был начать движение, ждем сигнала концевика
    next_state(&a_centering::wait_limit_search_done);
}

void a_centering::wait_limit_search_done() noexcept
{
    // Ждем срабатывыния концевика, всегда одного и того же
    if (!cnc_.is_limit(MARKER_AXIS, MARKER_LIMIT))
    {
        // Проверяем, нет ли срабатывания концевика движущейся оси
        if (cnc_.is_limit(task_.axis, task_.dir.back()))
            set_error(fmt::format("Не удалось найти заготовку по оси {}", task_.axis));
        
        return;
    }
    
    hw_.log_message(LogMsg::Info, "centering", fmt::format("Центр по оси {} найден", task_.axis));
    
    next_state(&a_centering::limit_search_done);
}

void a_centering::limit_search_done() noexcept
{
    proc_axis_ = '\0';

    // Запоминаем текущую позицию
    task_.rpos.push_back(cnc_.pos(task_.axis));

    // Концевик установлен, переключаемся в координатный режим
    next_state(&a_centering::switch_to_target_mode);
}

void a_centering::switch_to_target_mode() noexcept
{
    auto [ done, error ] = cnc_.switch_to_target_mode(task_.axis);
    if (!dhresult::check(done, error, [this] { set_error(fmt::format("Не удалось переключить режим оси {}", task_.axis)); }))
        return;

    // Удаляем проверенное направление из списка
    int const dir = task_.dir.back(); 
    task_.dir.pop_back();

    if (!task_.dir.empty())
    {
        // Если сходили только в одну сторону, возвращаемся на точку старта
        move_to_pos_ = start_point_pos_; 
    }
    else
    {
        // Если проверили обе точки
        if (task_.rpos.size() == 2)
        {
            // Рассчитываем центр и едем в него
            move_to_pos_ = (task_.rpos[1] - task_.rpos[0]) / 2 + task_.rpos[0];
        }
        else
        {
            // Значит это зуб и наша целевая позиция задается из конфигурации
            move_to_pos_ = cnc_.pos(task_.axis) + tooth_shift_ * (dir * -1);
        }
    }

    // Запоминаем активированную ось чтобы можно было прервать
    proc_axis_ = task_.axis;

    next_state(&a_centering::move_to_point);
}

void a_centering::move_to_point() noexcept
{
    // Настройки калибруемой оси
    auto const& cfg = axis_cfg_.axis(task_.axis);

    auto [ done, error ] = cnc_.target_move(task_.axis, move_to_pos_, cfg.speed[1]);
    if (!dhresult::check(done, error, [this] { set_error("Ошибка 1"); }))
        return;

    // Команда принята, переходим на ожидание выполнения
    next_state(&a_centering::wait_move_to_point_done);
}

void a_centering::wait_move_to_point_done() noexcept
{
    if (cnc_.status() == engine::cnc_status::run)
        return;
    next_state(&a_centering::move_to_point_done);
}

void a_centering::move_to_point_done() noexcept
{
    proc_axis_ = '\0';

    if (task_.dir.empty())
    {
        // Переходим к следующей оси
        next_state(&a_centering::init_next_axis);
    }
    else
    {
        // Сбрасываем контроллер касания
        hw_.set_flag(flags::do_reset_bki, true);
        cnc_.set_fake_limit(MARKER_AXIS, MARKER_LIMIT);

        // Меняем направление поиска и повторяем все шаги для обратного поиска
        hw_.log_message(LogMsg::Info, "centering", "Ожидание сброса БКИ");
        next_state(&a_centering::wait_reset_bki);
    }
}

void a_centering::set_error(std::string const &emsg) noexcept
{
    aem::log::error("a_centering::set_error: {}", emsg);
    emsg_ = emsg;
    next_state(nullptr);
}
