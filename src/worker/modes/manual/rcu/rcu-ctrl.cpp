#include "rcu-ctrl.h"

#include <axis-cfg.h>
#include <hardware.h>

rcu_ctrl::rcu_ctrl(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("rcu-ctrl")
    , base_ctrl(name(), hw, axis_cfg)
    , cnc_(hw.get_unit_by_class("", "cnc"))
    , rcu_led_(hw.get_unit_by_class("rcu-ctrl", "led"), "rcu-ctrl")
{ 
    hw.LSET.add(name(), "spin", [this](nlohmann::json const &value) 
    {
        int pos = value.get<int>();

        // Инициализируем внутренее состояние
        if (!pos_.has_value())
        {
            pos_ = pos;
            return;
        }
        
        int shift = pos - pos_.value();
        pos_ = pos;
         
        hw_.log_message(LogMsg::Command, name(), fmt::format("Относительное движение на {}", shift));
        spin_ += shift;
    });

    // Выбор оси на пульте, возможные значения (0, 'X','Y','Z','4','5','6')
    char akeys[] = { 'X','Y','Z','4','5','6' };
    for (auto akey : akeys)
    {
        akeys_[akey] = false;
        
        hw.LSET.add(name(), fmt::format("axis-{}", akey), [akey, this](nlohmann::json const &value) 
        {
            auto &state = akeys_[akey];
            bool flag = value.get<bool>();
            if (state == flag) return;
            state = flag;
            set_axis_tsp();
        });
    }
}

void rcu_ctrl::set_axis_tsp() noexcept
{
    // Выбор оси на пульте, возможные значения (0, 'X','Y','Z','4','5','6')
    axis_tsp_ = '0';
    for (auto const &akey : akeys_)
    {
        if (!akey.second) continue;
        axis_tsp_ = akey.first;
        break;
    }
    hw_.log_message(LogMsg::Command, name(), fmt::format("Текущая ось RCU: {}", axis_tsp_));
}

void rcu_ctrl::on_activate() noexcept
{
    last_axis_ = 0;
    last_shift_ = 0;

    // Сбрасываем значение
    spin_ = 0;

    update_internal_axis_cfg();

    next_state(&rcu_ctrl::turn_on_led);
}

void rcu_ctrl::on_deactivate() noexcept
{
    aem::log::trace("rcu_ctrl::on_deactivate");

    shift_ = 0;
    axis_ = '\0';

    next_state(&rcu_ctrl::stop_target_move);
}

void rcu_ctrl::turn_on_led() noexcept
{
    auto [ done, error ] = rcu_led_.set(false);
    if (!dhresult::check(done, error, [this] { next_state(&rcu_ctrl::ctrl_allow); }))
        return;
    next_state(&rcu_ctrl::ctrl_allow);
}

void rcu_ctrl::ctrl_allow() noexcept
{
    check_new_axis_cfg();

    if (axis_ != last_axis_ && last_axis_ != 0)
    {
        next_state(&rcu_ctrl::stop_target_move);
        return;
    }

    // Диск не менял положения или
    // Диск изменил положение, определяем для какой оси и
    // с какой скоростью необходимо осуществить перемещение
    if (spin_ == 0 || axis_ == 0)
        return;

    // Запоминаем и сбрасываем значение
    shift_ = spin_;
    spin_ = 0;

    // Если направление или ось изменились, сначала выполняем команду остановки
    if (((shift_ < 0) && (last_shift_ > 0)) || ((last_shift_ < 0) && (shift_ > 0)))
        next_state(&rcu_ctrl::stop_target_move);
    else
        next_state(&rcu_ctrl::prepare_target_move);
}

void rcu_ctrl::prepare_target_move() noexcept
{
    // Проверяем, в каком состоянии ось
    if (cnc_.is_in_independent_mode(axis_))
    {
        next_state(&rcu_ctrl::switch_to_target_mode);
        return;
    }
    next_state(&rcu_ctrl::relative_target_move);
}

void rcu_ctrl::switch_to_target_mode() noexcept
{
    auto [ done, error ] = cnc_.switch_to_target_mode(axis_);
    if (!dhresult::check(done, error, [this] { next_state(&rcu_ctrl::ctrl_error); }))
        return;

    // Переключились, отдаем команду движения
    next_state(&rcu_ctrl::relative_target_move);
}

void rcu_ctrl::relative_target_move() noexcept
{
    if (cnc_.N() < 2)
    {
        float shift = get_real_shift_from_cfg(axis_, shift_); 

        auto [ done, error ] = cnc_.relative_target_move(axis_, shift, speed_[axis_]);
        if (!dhresult::check(done, error, [this] { next_state(&rcu_ctrl::ctrl_error); }))
            return;
    }

    // Команда принята, запоминаем ее параметры
    last_axis_ = axis_;
    last_shift_ = shift_;
    shift_ = 0;

    // Мы можем не дождаться очередного ответа
    cnc_.reset();
    
    next_state(&rcu_ctrl::ctrl_allow);
}

void rcu_ctrl::stop_target_move() noexcept
{
    auto [ done, error ] = cnc_.stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&rcu_ctrl::ctrl_error); }))
        return;
    next_state(&rcu_ctrl::wait_target_move_stop);
}

void rcu_ctrl::wait_target_move_stop() noexcept
{
    if (cnc_.status() == engine::cnc_status::idle)
    {
        // Нужно для движения в обратную сторону, если перед началом нового 
        // движения нам потребовалось остановить текущее
        if (shift_ != 0 && axis_ != '\0')
        {
            next_state(&rcu_ctrl::prepare_target_move);
            return;
        }

        last_axis_ = '\0';
        last_shift_ = 0;

        next_state(is_active() ? &rcu_ctrl::ctrl_allow : nullptr);
        return;
    }

    if (cnc_.status() != engine::cnc_status::queue)
        return;

    next_state(&rcu_ctrl::reset_queue);
}

void rcu_ctrl::reset_queue() noexcept
{
    auto [ done, error ] = cnc_.hard_stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&rcu_ctrl::ctrl_error); }))
        return;
    next_state(&rcu_ctrl::wait_target_move_stop);
}

void rcu_ctrl::ctrl_error() noexcept
{
    hw_.log_message(LogMsg::Error, name(), "Ошибка при работе с контроллером");

    last_axis_ = '\0';
    last_shift_ = 0;

    next_state(&rcu_ctrl::ctrl_allow);
}

float rcu_ctrl::get_real_shift_from_cfg(char axis, int ishift) const noexcept
{
    auto const& cfg = axis_cfg_.axis(axis);

    // Проверка на ноль значения коэфициента перевода в миллиметры
    float const ratio = std::lround(cfg.rcu.ratio * 1000) ? cfg.rcu.ratio : 1.f;

    return ishift / ratio;
}

void rcu_ctrl::update_internal_axis_cfg() noexcept
{
    real_axis_tsp_ = axis_tsp_;
    axis_ = '\0';

    // Каждый раз при смене оси сбрасываем последнее смещение
    spin_ = 0;

    if (axis_tsp_ != '\0')
    {
        for (auto const& axis : axis_cfg_)
        {
            if (!axis.second.use() || axis.second.rcu.link != axis_tsp_)
                continue;
            axis_ = axis.first;
            break;
        }
    }

    hw_.notify({ "sys", "ctrl-mode-axis" }, axis_);

    base_ctrl::update_internal_axis_cfg();
}

bool rcu_ctrl::is_new_axis_cfg() const noexcept
{
    return base_ctrl::is_new_axis_cfg() || (axis_tsp_ != real_axis_tsp_);
}

float rcu_ctrl::get_speed(we::axis_desc const &ad, std::size_t tsp) const noexcept
{
    return ad.rcu.speed[tsp];
}
