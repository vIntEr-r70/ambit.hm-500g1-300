#include "tooth-center-search.hpp"

tooth_center_search::tooth_center_search()
    : eng::sibus::node("tooth-center-search")
{
    iwire_ = node::add_input_wire();

    x_.owire = node::add_output_wire("axis-motion-X");
    y_.owire = node::add_output_wire("axis-motion-Y");
    owire_bki_ = node::add_output_wire("bki-ctl");

    // node::on_target_state(x_.owire, [this]() {
    // });
    // node::when_target_ready(y_.owire, [this](eng::abc::pack args) {
    // });
    // node::when_target_ready(owire_bki_, [this](eng::abc::pack args) {
    // });
    //
    // node::target_notify(x_.owire, [this](eng::abc::pack args) {
    // });
    // node::when_target_ready(y_.owire, [this](eng::abc::pack args) {
    // });
    // node::when_target_ready(owire_bki_, [this](eng::abc::pack args) {
    // });


    // Входящее управление прервалось, просто освобождаем оси
    // Вызывается если связь с узлом источником прервалась либо
    // узел сам прекратил активность
    // node::on_source_deactivated(iwire_, [this]
    // {
    //     node::release_target(x_.owire);
    //     node::release_target(y_.owire);
    //     node::release_target(owire_bki_);
    // });
    //
    // // Команды управления от контроллера
    // node::signal_from_source(iwire_, [this](eng::abc::pack args)
    // {
    //     // Параметры процесса
    //     deep_ = eng::abc::get<double>(args);
    //     node::wire_signal_done(iwire_, { });
    //
    //     // Запускаем алгоритм центровки на выполнение
    //     capture_axis();
    // });
}

void tooth_center_search::register_on_bus_done()
{
    // // Желание воспользоватья ресурсом
    // axis_[axis::step].task_session_id = node::task_user_attach(axis_[axis::step].task_user_id, eng::sibus::task_session_id_t{ 0 });
    // axis_[axis::deep].task_session_id = node::task_user_attach(axis_[axis::deep].task_user_id, eng::sibus::task_session_id_t{ 0 });
}

// Забираем оси X и Y под управление 
void tooth_center_search::capture_axis()
{
    // node::capture_target(x_.owire);
    // node::capture_target(y_.owire);
    // node::capture_target(owire_bki_);
    //
    // // Если хоть с одним нету связи то работать нет возможности
    // if (!node::is_wire_online(owire_x_) || !node::is_wire_online(owire_y_))
    // {
    //     // Переходим в состояние ошибки в процессе выполнения
    //     node::wire_notify(iwire_, { "error" });
    //     return;
    // }
    //
    // // Даем команду на захват осей
    // node::wire_activate(owire_x_);
    // node::wire_activate(owire_y_);
    //
    // // Помечаем что ждем ответа от обеих осей
    // wait_for_x_ready_ = true;
    // wait_for_y_ready_ = true;
    //
    // // Ждем пока обе оси будут захвачены
    // step_ = &tooth_center_search::capture_axis_done;
}

void tooth_center_search::activate_axis_done()
{
    // // Если обе оси готовы к работе, продолжаем
    // // выполнение алгоритма. Иначе сообщаем об аварии
    // if (!node::is_wire_active(owire_x_) || !node::is_wire_active(owire_y_))
    // {
    //     // Обе оси ответили и они не готовы, ошибка
    //     if (wait_for_x_ready_ && wait_for_y_ready_)
    //     {
    //         // Переходим в состояние ошибки в процессе выполнения
    //         node::wire_notify(iwire_, { "error" });
    //     }
    //     return;
    // }
    //
    // // Едем по оси X в положительном направлении пока не упремся в деталь
    // node::send_wire_signal(owire_x_, { "spin", x_.speed });
    // step_ = &tooth_center_search::move_x_point_1_done;
}

// Произошло касание детали в точке 1 по оси X
void tooth_center_search::move_x_point_1_done()
{
    // // Все нормально, возвращаемся обратно в точку старта
    // if (x_.status == "idle" && x_.bki)
    // {
    //     node::send_wire_signal(owire_x_, { "nobki-move-to", x_zero_pos_, x_.speed });
    //     step_ = &tooth_center_search::move_x_point_0_done;
    //     return;
    // }
}

// Мы в начальной позиции, необходимо сбросить БКИ
void tooth_center_search::move_x_point_0_done()
{
    if (x_.status == "idle" && x_.bki)
    {
        node::send_wire_signal(owire_bki_, { });
        step_ = &tooth_center_search::bki_x_point_1_reset;
        return;
    }
}

// БКИ сброшен и готов к новому срабатыванию
void tooth_center_search::bki_x_point_1_reset()
{
    // // Сбрасываем БКИ и продолжаем
    // if (x_.status == "idle" && !x_.bki)
    // {
    //     // Едем по оси X в отрицательном направлении пока не упремся в деталь
    //     node::send_wire_signal(owire_x_, { "spin", -x_.speed });
    //     step_ = &tooth_center_search::move_x_point_2_done;
    //     // Ждем когда движение будет прервано
    //     return;
    // }
}

// Произошло касание детали в точке 2 по оси X
void tooth_center_search::move_x_point_2_done()
{
    // // Рассчитываем координату центра и едем туда
    // if (x_.status == "idle" && x_.bki)
    // {
    //     double x_center_pos = 0.0;
    //     node::send_wire_signal(owire_x_, { "nobki-move-to", x_center_pos, x_.speed });
    //     step_ = &tooth_center_search::move_x_center_done;
    //     return;
    // }
}

void tooth_center_search::move_x_center_done()
{
    // // Все нормально, сбрасываем БКИ и продолжаем
    // if (x_.status == "idle" && x_.bki)
    // {
    //     node::send_wire_signal(owire_bki_, { });
    //     step_ = &tooth_center_search::bki_x_center_reset;
    //     return;
    // }
}

// БКИ сброшен и готов к новому срабатыванию
void tooth_center_search::bki_x_center_reset()
{
    // // Инициируем движение второй оси
    // if (x_.status == "idle" && !x_.bki)
    // {
    //     node::send_wire_signal(owire_y_, { "spin", std::copysign(y_.speed, deep_) });
    //     step_ = &tooth_center_search::move_y_deep_done;
    //     return;
    // }
}

void tooth_center_search::move_y_deep_done()
{
    // if (y_.status == "idle" && y_.bki)
    // {
    //     node::send_wire_signal(owire_y_, { "nobki-shift", deep_ });
    //     step_ = &tooth_center_search::move_y_target_done;
    //     return;
    // }
}

void tooth_center_search::move_y_target_done()
{
    // Сбрасываем БКИ
    if (y_.status == "idle" && y_.bki)
    {
        node::send_wire_signal(owire_bki_, { });
        step_ = &tooth_center_search::bki_y_target_reset;
        return;
    }
}

void tooth_center_search::bki_y_target_reset()
{
    // // Мы в целевой точке, завершаем работу алгоритма
    // if (y_.status == "idle" && !y_.bki)
    // {
    //     // Освобождаем оси, они нам больше не нужны
    //     node::wire_deactivate(owire_x_);
    //     node::wire_deactivate(owire_y_);
    //     step_ = &tooth_center_search::bki_y_target_reset;
    //     return;
    // }
}

void tooth_center_search::deactivate_axis_done()
{
    // // Если обе оси готовы к работе, продолжаем
    // // выполнение алгоритма. Иначе сообщаем об аварии
    // if (node::is_wire_active(owire_x_) || node::is_wire_active(owire_y_))
    // {
    //     // Обе оси ответили и они не готовы, ошибка
    //     if (wait_for_x_ready_ && wait_for_y_ready_)
    //     {
    //         // Переходим в состояние ошибки в процессе выполнения
    //         node::wire_notify(iwire_, { "error" });
    //     }
    //     return;
    // }
    //
    // // Едем по оси X в положительном направлении пока не упремся в деталь
    // node::send_wire_signal(owire_x_, { "spin", x_.speed });
    // step_ = &tooth_center_search::move_x_point_1_done;
}

