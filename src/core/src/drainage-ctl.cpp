#include "drainage-ctl.hpp"
#include "eng/sibus/sibus.hpp"

#include <eng/log.hpp>

// Данная система контроллирует уровень жидкости в некой емкости
// основываясь на значениях датчиков уровня.
// Поведение:
// Датчик max: включаем насос
// Датчик min: выключаем насос

drainage_ctl::drainage_ctl()
    : eng::sibus::node("drainage-ctl")
{
    // Сигналы с датчиков
    node::add_input_port("PMAX", [this](eng::abc::pack args)
    {
        pmax_ = eng::abc::get<bool>(args);
        eng::log::info("{}: PMAX = {}", name(), *pmax_);
        (this->*state_)();
    });
    node::add_input_port("PMIN", [this](eng::abc::pack args)
    {
        pmin_ = eng::abc::get<bool>(args);
        eng::log::info("{}: PMIN = {}", name(), *pmin_);
        (this->*state_)();
    });

    // Сигнал управления насосом
    pump_ = node::add_output_port("H");

    state_ = &drainage_ctl::level_norm;
}

void drainage_ctl::level_norm()
{
    if (pmax_ && pmax_.value())
    {
        eng::log::info("{}: Уровень достиг максимума", name());

        // Включаем насос
        node::set_port_value(pump_, { true });

        state_ = &drainage_ctl::level_max;

        return;
    }

    if (pmin_ && pmin_.value())
    {
        eng::log::info("{}: Уровень достиг минимума", name());

        // Выключаем насос
        node::set_port_value(pump_, { false });

        // Обновляем внутренее состояние
        state_ = &drainage_ctl::level_min;

        return;
    }
}

// Откачиваем воду до срабатывания нижнего датчика
// void drainage_ctl::pumping_out()
// {
//     if (pmin_ && pmin_.value())
//         return;
// }


void drainage_ctl::level_min()
{
    if (pmin_ && pmin_.value())
        return;

    if (pmax_ && !pmax_.value())
        return;

    state_ = &drainage_ctl::level_norm;
}

void drainage_ctl::level_max()
{
    if (pmax_ && pmax_.value())
        return;

    if (pmin_ && !pmin_.value())
        return;

    state_ = &drainage_ctl::level_norm;
}

