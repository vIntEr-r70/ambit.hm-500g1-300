#include "drainage-ctl.hpp"
#include "eng/sibus/sibus.hpp"

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
        (this->*pump_state_)();
    });
    node::add_input_port("PMIN", [this](eng::abc::pack args)
    {
        pmin_ = eng::abc::get<bool>(args);
        (this->*pump_state_)();
    });

    // Сливаем в канализацию
    node::add_input_port("drainage", [this](eng::abc::pack args)
    {
        // if (eng::abc::get<bool>(args))
        //     set_valve_position(valve::drainage);
    });

    // Сливаем в бочки
    node::add_input_port("barrels", [this](eng::abc::pack args)
    {
        // if (eng::abc::get<bool>(args))
        //     set_valve_position(valve::barrels);
    });

    // Сигнал управления насосом
    pump_ = node::add_output_port("pump");

    // Провод управления клапаном выбора куда
    // насос будет откачивать жидкость
    va_ctl_ = node::add_output_wire();
    node::set_wire_status_handler(va_ctl_, [this] {
        // update_valve_state();
    });

    // va_ctl_ = node::add_output_wire();
    // node::set_wire_status_handler(va_ctl_, [this] {
    //     (this->*pump_)();
    // });

    pump_state_ = &drainage_ctl::turned_off;
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
    node::set_port_value(pump_, { true });

    // Обновляем внутренее состояние
    pump_state_ = &drainage_ctl::turned_on;
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
    node::set_port_value(pump_, { false });

    // Обновляем внутренее состояние
    pump_state_ = &drainage_ctl::turned_off;
}

void drainage_ctl::set_valve_position(vstate state)
{
    // Если нам известно состояние задвижки
    if (valve_state_)
    {
        // Передаем ответственность за принятие решения ему
        (this->*valve_state_)(state);
    }
    else
    {
        // Если ответ нам еще не вернулся, запоминаем чего хотел оператор
        next_valve_state_ = state;
    }
}

// Происходит позиционирование на канализацию
void drainage_ctl::valve_move_to_drainage(vstate state)
{
    if (state == vstate::drainage)
        return;

    target_valve_state_ = state;
    node::activate(va_ctl_, { false, true });

    valve_state_ = nullptr;
}

// Происходит позиционирование на бочки
void drainage_ctl::valve_move_to_barrels(vstate state)
{
    if (state == vstate::barrels)
        return;

    target_valve_state_ = state;
    node::activate(va_ctl_, { true, false });

    valve_state_ = nullptr;
}

// Находимся в позиции на канализацию
void drainage_ctl::valve_in_drainage(vstate state)
{
    valve_move_to_drainage(state);
}

// Находимся в позиции на бочки
void drainage_ctl::valve_in_barrels(vstate state)
{
    valve_move_to_barrels(state);
}

// Состояние обновилось, синхронизируемся
void drainage_ctl::update_valve_state(eng::sibus::istatus istatus)
{
    switch(istatus)
    {
    // Мы открываемся куда была отправлена команда
    case eng::sibus::istatus::active:
        valve_state_ = target_valve_state_ == vstate::barrels ?
            &drainage_ctl::valve_move_to_barrels : &drainage_ctl::valve_move_to_drainage;
        break;
    // Мы завершили операцию и находимся в требуемой позиции
    case eng::sibus::istatus::ready:
        valve_state_ = target_valve_state_ == vstate::barrels ?
            &drainage_ctl::valve_in_barrels : &drainage_ctl::valve_in_drainage;
        break;
    // Случилась какая-то неведомая херота
    case eng::sibus::istatus::blocked:
        // ??? Это критичная ситуация
        next_valve_state_.reset();
        return;
    }

    if (!next_valve_state_)
        return;

    (this->*valve_state_)(*next_valve_state_);
    next_valve_state_.reset();
}


