#include "rcu-ctl.hpp"

// Просто собираем цепь на основании значения входного контакта
// Связь всегда один к одному либо отсутствует

rcu_ctl::rcu_ctl()
    : eng::sibus::node("rcu-ctl")
{
    // Текущая выбранная ось для взаимодействия
    add_input_port("axis", [this](eng::abc::pack args)
    {
        char axis = eng::abc::get<char>(args, 0);
        switch_axis_wire(axis);
    });

    iwire_ = add_input_wire();

    owires_['X'] = add_output_wire("X");
    owires_['Y'] = add_output_wire("Y");
    owires_['Z'] = add_output_wire("Z");
    owires_['V'] = add_output_wire("V");
}

// Перекомутируем внутреннее состояние согласно новой оси
void rcu_ctl::switch_axis_wire(char axis)
{
    // Если ось не изменилась, ничего не меняем
    if (axis == axis_) return;

    // Если у нас в данный момент какая-то ось активна, разрываем связь с этой осью
    if (axis_ != '\0') node::drop_wire(iwire_);

    // Создаем новую связь
    node::link_wire(iwire_, owires_[axis]);

    // Запоминаем ось
    axis_ = axis;
}

