#include "drainage-ctl.hpp"

#include <eng/log.hpp>

// Данная система контроллирует уровень жидкости в некой емкости
// основываясь на значениях датчиков уровня.
// Поведение:
// Датчик max: включаем насос
// Датчик min: выключаем насос
// Так-же в системе присутствует клапан, который может
// находится в состояниях [подача в канализацию, переключение, подача в емкость]

drainage_ctl::drainage_ctl()
    : eng::sibus::node("drainage-ctl")
{
    // Сигналы с датчиков
    node::add_input_port("PMAX", [this](eng::abc::pack args)
    {
        pmax_ = eng::abc::get<bool>(args);
        (this->*pump_)();
    });
    node::add_input_port("PMIN", [this](eng::abc::pack args)
    {
        pmin_ = eng::abc::get<bool>(args);
        (this->*pump_)();
    });

    // Сигнал управления насосом
    pump_out_ = node::add_output_port("H");

    va_ctl_ = node::add_output_wire();
    node::set_wire_status_handler(va_ctl_, [this] {
        (this->*pump_)();
    });

    pump_ = &drainage_ctl::turned_off;
}

void drainage_ctl::turned_off()
{
    // Нет необходимости включать насос
    if (!pmax_ || !pmax_.value())
        return;

    eng::log::info("{}: Уровень достиг максимума", name());

    // Включение насоса заблокировано задвижкой
    if (!node::is_blocked(va_ctl_) && !node::is_ready(va_ctl_))
    {
        eng::log::info("{}: Задвижка блокирует включение насоса", name());
        return;
    }

    eng::log::info("{}: Включаем насос", name());

    // Включаем насос
    node::set_port_value(pump_out_, { true });

    // Обновляем внутренее состояние
    pump_ = &drainage_ctl::turned_on;
}

void drainage_ctl::turned_on()
{
    // Если задвижка не производит переключений
    if (!node::is_active(va_ctl_))
    {
        // Если насос должен продолжать работать
        if (!pmin_ || !pmin_.value())
            return;

        eng::log::info("{}: Уровень достиг минимума", name());
    }
    else
    {
        eng::log::info("{}: Задвижка пришла в движение", name());
    }

    eng::log::info("{}: Выключаем насос", name());

    // Выключаем насос
    node::set_port_value(pump_out_, { false });

    // Обновляем внутренее состояние
    pump_ = &drainage_ctl::turned_off;
}

