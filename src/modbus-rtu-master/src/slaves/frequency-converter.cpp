#include "frequency-converter.hpp"

#include <eng/timer.hpp>
#include <eng/log.hpp>
#include <eng/json.hpp>
#include <eng/sibus/client.hpp>


frequency_converter::frequency_converter(std::size_t unit_id)
    : eng::sibus::node{ "fc-ctl" }
    , modbus_unit(unit_id)
    , hw_state_(nullptr)
{
    p_out_[pout::powered] = node::add_output_port("powered");
    p_out_[pout::online] = node::add_output_port("online");
    p_out_[pout::damaged] = node::add_output_port("damaged");

    p_out_[pout::F] = node::add_output_port("F");
    p_out_[pout::P] = node::add_output_port("P");
    p_out_[pout::I] = node::add_output_port("I");
    p_out_[pout::U_in] = node::add_output_port("U-in");
    p_out_[pout::U_out] = node::add_output_port("U-out");

    ictl_ = node::add_input_wire("");

    node::set_activate_handler(ictl_, [this](eng::abc::pack args) {
        activate(std::move(args));
    });

    node::set_deactivate_handler(ictl_, [this] { deactivate(); });

    // // Обработчик входящих сигналов
    // node::set_wire_request_handler(ictl_, [this](eng::abc::pack args) {
    //     start_command_execution(std::move(args));
    // });

    std::string key{ "fc/current-conductors" };
    eng::sibus::client::config_listener(key, [this](std::string_view json)
    {
        eng::json::object cfg(json);

        auto id = cfg.get<std::size_t>("mask", 0);

        eng::json::object item(cfg.get_array("records")[id]);
        I_max_ = item.get<double>("I-max", 0.0);
        iup_[1] = item.get<double>("U-max", 0.0);
    });

    std::size_t idx;

    // Опрос текущего состояния устройства
    idx = modbus_unit::add_read_task(0xA411, 5, 300);
    read_task_handlers_[idx] = &frequency_converter::read_status_done;

    // Опрос частоты
    idx = modbus_unit::add_read_task(0xA430, 1, 1000);
    read_task_handlers_[idx] = &frequency_converter::read_F_done;

    // Опрос входного и выходного напряжений и тока
    idx = modbus_unit::add_read_task(0xA432, 3, 1000);
    read_task_handlers_[idx] = &frequency_converter::read_UUI_done;

    // Опрос мощности
    idx = modbus_unit::add_read_task(0xA437, 1, 1000);
    read_task_handlers_[idx] = &frequency_converter::read_P_done;
}

// void frequency_converter::start_command_execution(eng::abc::pack args)
// {
//     if (offline_)
//     {
//         node::wire_response(ictl_, false, { "Отсутствует связь" });
//         return;
//     }
//
//     static std::unordered_map<std::string_view, commands_handler> const map {
//         { "start",  &frequency_converter::cmd_start  },
//         { "stop",   &frequency_converter::cmd_stop   },
//         { "set-i",  &frequency_converter::cmd_set_i  },
//         { "set-p",  &frequency_converter::cmd_set_p  },
//     };
//
//     std::string_view cmd = eng::abc::get<std::string_view>(args, 0);
//     auto it = map.find(cmd);
//     if (it == map.end())
//     {
//         node::wire_response(ictl_, false, { std::format("Незнакомая комманда: {}", cmd) });
//         return;
//     }
//
//     if (read_internal_tid_)
//         eng::timer::kill_timer(read_internal_tid_);
//
//     args.pop_front();
//
//     (this->*(it->second))(std::move(args));
// }

/////////////////////////////////////////////////////////////////////////////////////////////

// Запускаем устройство в работу
void frequency_converter::activate(eng::abc::pack args)
{
    if (!is_online())
    {
        // node::deactivated(ictl_, { "Отсутствует связь с устройством" });
        node::deactivated(ictl_);
        return;
    }

    if (hw_state_ == &frequency_converter::s_damaged)
    {
        // node::deactivated(ictl_, { "Устройство в состоянии аварии" });
        node::deactivated(ictl_);
        return;
    }

    if (hw_state_ == &frequency_converter::s_powered)
    {
        node::activated(ictl_);
        return;
    }

    // Записываем уставки
    std::array<std::uint16_t, 3> iup{
        static_cast<std::uint16_t>(std::lround(iup_[0] * 10.0f)),
        static_cast<std::uint16_t>(std::lround(iup_[1])),
        static_cast<std::uint16_t>(std::lround(iup_[2] * 0.01f))
    };

    modbus_unit::write_multiple(0xA420, { iup.data(), iup.size() });

    // Передаем команду на запуск
    write_task_handler_ = {
        modbus_unit::write_single(0xA410, 0x0001),
        &frequency_converter::w_start
    };
}

// Останавливаем устройство
void frequency_converter::deactivate()
{
    if (!is_online())
    {
        // node::deactivated(ictl_, { "Отсутствует связь с устройством" });
        node::deactivated(ictl_);
        return;
    }

    if (hw_state_ == &frequency_converter::s_idle)
    {
        node::deactivated(ictl_);
        return;
    }

    std::uint16_t command = 0x0000;
    if (hw_state_ == &frequency_converter::s_damaged)
        command = 0x0002;

    // Передаем команду на остановку или сброс аварии
    write_task_handler_ = {
        modbus_unit::write_single(0xA410, command),
        &frequency_converter::w_stop
    };
}

/////////////////////////////////////////////////////////////////////////////////////////////

void frequency_converter::s_idle(std::uint16_t status)
{
    switch(status)
    {
    case 0:
        return;
    case 1:
        node::set_port_value(p_out_[pout::powered], { true });
        hw_state_ = &frequency_converter::s_powered;
        return;
    }

    node::set_port_value(p_out_[pout::damaged],
        { true, damages_[0], damages_[1], damages_[2], damages_[3] });

    hw_state_ = &frequency_converter::s_damaged;
}

void frequency_converter::s_powered(std::uint16_t status)
{
    switch(status)
    {
    case 0:
        node::set_port_value(p_out_[pout::powered], { false });
        hw_state_ = &frequency_converter::s_idle;
        node::deactivated(ictl_);
        return;
    case 1:
        return;
    }

    node::deactivated(ictl_);

    node::set_port_value(p_out_[pout::damaged],
        { true, damages_[0], damages_[1], damages_[2], damages_[3] });

    node::set_port_value(p_out_[pout::powered], { false });

    hw_state_ = &frequency_converter::s_damaged;
}

void frequency_converter::s_damaged(std::uint16_t status)
{
    switch(status)
    {
    case 0:
        node::set_port_value(p_out_[pout::damaged], { false });
        node::set_port_value(p_out_[pout::powered], { false });
        hw_state_ = &frequency_converter::s_idle;
        return;
    case 1:
        node::set_port_value(p_out_[pout::damaged], { false });
        node::set_port_value(p_out_[pout::powered], { true });
        hw_state_ = &frequency_converter::s_powered;
        return;
    }

    node::set_port_value(p_out_[pout::damaged],
        { true, damages_[0], damages_[1], damages_[2], damages_[3] });
}

/////////////////////////////////////////////////////////////////////////////////////////////

void frequency_converter::now_unit_online()
{
    eng::log::info("frequency_converter::now_unit_online");

    modbus_unit::write_single(0xA410, 0x0002);
    node::set_port_value(p_out_[pout::online], { true });
}

void frequency_converter::connection_was_lost()
{
    if (hw_state_ == &frequency_converter::s_powered)
    {
        // node::deactivated(ictl_, { "Связь с устройством потеряна" });
        node::deactivated(ictl_);
    }

    hw_state_ = nullptr;
    write_task_handler_.handler = nullptr;

    std::ranges::fill(damages_, 0);

    node::set_port_value(p_out_[pout::powered], { });
    node::set_port_value(p_out_[pout::damaged], { });

    node::set_port_value(p_out_[pout::F], { });
    node::set_port_value(p_out_[pout::U_in], { });
    node::set_port_value(p_out_[pout::U_out], { });
    node::set_port_value(p_out_[pout::I], { });
    node::set_port_value(p_out_[pout::P], { });

    node::set_port_value(p_out_[pout::online], { false });
}

/////////////////////////////////////////////////////////////////////////////////////////////

void frequency_converter::write_task_done(std::size_t idx)
{
    // Если логика в режиме ожидания завершения операции
    if (write_task_handler_.handler && write_task_handler_.idx == idx)
    {
        (this->*write_task_handler_.handler)(true);
        write_task_handler_.handler = nullptr;
    }
}

void frequency_converter::w_start(bool success)
{
    if (success)
        node::activated(ictl_);
    else
    {
        // node::deactivated(ictl_, { "Не удалось включить устройство" });
        node::deactivated(ictl_);
    }
}

void frequency_converter::w_stop(bool success)
{
    // node::deactivated(ictl_, { "Не удалось выключить устройство" });
    node::deactivated(ictl_);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void frequency_converter::read_task_done(std::size_t idx, readed_regs_t regs)
{
    (this->*read_task_handlers_[idx])(regs);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void frequency_converter::read_status_done(readed_regs_t regs)
{
    std::uint16_t status = regs[0];

    if (status == 2 && hw_state_ == &frequency_converter::s_damaged)
    {
        auto src = regs.subspan(1);
        if (std::ranges::equal(src, damages_))
            return;
        std::ranges::copy(src, damages_.begin());
    }
    else if (hw_state_ == nullptr)
    {
        hw_state_ = &frequency_converter::s_damaged;
        std::ranges::copy(regs.subspan(1), damages_.begin());
    }

    (this->*hw_state_)(status);
}

void frequency_converter::read_F_done(readed_regs_t regs)
{
    double F = regs[0] * 10.0;
    node::set_port_value(p_out_[pout::F], { F });
}

void frequency_converter::read_UUI_done(readed_regs_t regs)
{
    double U_in = regs[0];
    node::set_port_value(p_out_[pout::U_in], { U_in });

    double U_out = regs[1];
    node::set_port_value(p_out_[pout::U_out], { U_out });

    double I = regs[2] * 0.1;
    node::set_port_value(p_out_[pout::I], { I });
}

void frequency_converter::read_P_done(readed_regs_t regs)
{
    double P = regs[0] * 100.0;
    node::set_port_value(p_out_[pout::P], { P });
}

/////////////////////////////////////////////////////////////////////////////////////////////

// Не удалось выполнить команду
// void frequency_converter::device_offline()
// {
//     node::wire_response(ictl_, false, { "Связь потеряна" });
//     set_offline_state();
//     init_next_read();
// }
//
// /////////////////////////////////////////////////////////////////////////////////////////////
//
// // Команда на запуск [ current, power ]
// void frequency_converter::cmd_start(eng::abc::pack args)
// {
//     iup_[0] = std::min(eng::abc::get<double>(args, 0), I_max_);
//     iup_[2] = eng::abc::get<double>(args, 1);
//     eng::log::info("frequency_converter::cmd_start: P: {}, I: {}", iup_[2], iup_[0]);
//
//     // Запрашиваем текущее состояние
//     modbus_rtu_slave::read_holding(0xA411, 3,
//         &frequency_converter::st_start_get_status, &frequency_converter::device_offline);
// }
//
// // Получен статус устройства
// void frequency_converter::st_start_get_status(std::span<std::uint16_t const> regs)
// {
//     // Если устройство в состоянии аварии
//     if (regs[0] == 2)
//     {
//         node::wire_response(ictl_, false, { "В состоянии аварии" });
//         init_next_read();
//         return;
//     }
//
//     // Уже запущены
//     if (regs[1] == 1)
//     {
//         node::wire_response(ictl_, true, { });
//         init_next_read();
//         return;
//     }
//
//     // Записываем уставки
//     std::array<std::uint16_t, 3> iup{
//         static_cast<std::uint16_t>(std::lround(iup_[0] * 10.0f)),
//         static_cast<std::uint16_t>(std::lround(iup_[1])),
//         static_cast<std::uint16_t>(std::lround(iup_[2] * 0.01f))
//     };
//     modbus_rtu_slave::write_multiple(0xA420, { iup.data(), iup.size() },
//         &frequency_converter::st_start_update_set, &frequency_converter::device_offline);
// }
//
// // Уставки записаны
// void frequency_converter::st_start_update_set()
// {
//     // Передаем команду на запуск
//     modbus_rtu_slave::write_single(0xA410, 0x0001,
//         &frequency_converter::st_start_check, &frequency_converter::device_offline);
// }
//
// // Команда на запуск передана
// void frequency_converter::st_start_check()
// {
//     // Запрашиваем текущее состояние
//     modbus_rtu_slave::read_holding(0xA411, 3,
//         &frequency_converter::st_start_wait_done, &frequency_converter::device_offline);
// }
//
// // Ждем когда устройство запустится
// void frequency_converter::st_start_wait_done(std::span<std::uint16_t const> regs)
// {
//     // Если устройство в состоянии аварии
//     if (regs[0] == 2)
//     {
//         node::wire_response(ictl_, false, { "Не удалось включить" });
//         init_next_read();
//         return;
//     }
//
//     // Уже запущены
//     if (regs[0] == 1)
//     {
//         node::wire_response(ictl_, true, { });
//         init_next_read();
//         return;
//     }
//
//     // Повторяем попытку получить состояние
//     eng::timer::once(std::chrono::milliseconds(100), [this]
//     {
//         st_start_check();
//     });
// }
//
// /////////////////////////////////////////////////////////////////////////////////////////////
//
// // Команда на прекращение работы
// void frequency_converter::cmd_stop(eng::abc::pack)
// {
//     eng::log::info("frequency_converter::cmd_stop");
//     st_stop_init();
// }
//
// void frequency_converter::st_stop_init()
// {
//     // Запрашиваем текущее состояние
//     modbus_rtu_slave::read_holding(0xA411, 3,
//         &frequency_converter::st_stop_get_status, &frequency_converter::device_offline);
// }
//
// // Получен статус устройства
// void frequency_converter::st_stop_get_status(std::span<std::uint16_t const> regs)
// {
//     // Уже остановлено
//     if (regs[0] == 0)
//     {
//         node::wire_response(ictl_, true, { });
//         init_next_read();
//         return;
//     }
//
//     // Если в состоянии аварии
//     if (regs[0] == 2)
//     {
//         // Передаем команду на сброс аварии
//         modbus_rtu_slave::write_single(0xA410, 0x0002,
//             &frequency_converter::st_stop_check, &frequency_converter::device_offline);
//
//         return;
//     }
//
//     // Передаем команду на остановку
//     modbus_rtu_slave::write_single(0xA410, 0x0000,
//         &frequency_converter::st_stop_check, &frequency_converter::device_offline);
// }
//
// // Команда на остановку / сброс аварии передана
// void frequency_converter::st_stop_check()
// {
//     // Запрашиваем текущее состояние
//     modbus_rtu_slave::read_holding(0xA411, 3,
//         &frequency_converter::st_stop_wait_done, &frequency_converter::device_offline);
// }
//
// // Ждем когда устройство остановится / сбросит аварию
// void frequency_converter::st_stop_wait_done(std::span<std::uint16_t const> regs)
// {
//     // Уже остановлено
//     if (regs[0] == 0 || regs[0] == 2)
//     {
//         node::wire_response(ictl_, true, { });
//         init_next_read();
//         return;
//     }
//
//     // Повторяем попытку получить состояние
//     eng::timer::once(std::chrono::milliseconds(100), [this]
//     {
//         st_stop_check();
//     });
// }
//
// /////////////////////////////////////////////////////////////////////////////////////////////
//
// // [ current ]
// void frequency_converter::cmd_set_i(eng::abc::pack args)
// {
//     iup_[0] = std::min(eng::abc::get<double>(args), I_max_);
//     st_update_set();
// }
//
// // [ power ]
// void frequency_converter::cmd_set_p(eng::abc::pack args)
// {
//     iup_[2] = eng::abc::get<double>(args);
//     st_update_set();
// }
//
// void frequency_converter::st_update_set()
// {
//     // Записываем уставки
//     std::array<std::uint16_t, 3> iup{
//         static_cast<std::uint16_t>(std::lround(iup_[0] * 10.0f)),
//         static_cast<std::uint16_t>(std::lround(iup_[1])),
//         static_cast<std::uint16_t>(std::lround(iup_[2] * 0.01f))
//     };
//     modbus_rtu_slave::write_multiple(0xA420, { iup.data(), iup.size() },
//         &frequency_converter::st_set_done, &frequency_converter::device_offline);
// }
//
// // Уставки записаны
// void frequency_converter::st_set_done()
// {
//     node::wire_response(ictl_, true, { });
//     init_next_read();
// }
//
// /////////////////////////////////////////////////////////////////////////////////////////////
//
// void frequency_converter::read_device_offline()
// {
//     set_offline_state();
//     init_next_read();
// }
//
// void frequency_converter::init_next_read()
// {
//     // if (node::touch_signal(iwire_))
//     //     return;
//     //
//     // node::enable_signals(iwire_);
//     //
//     // Повторяем попытку получить состояние
//     read_internal_tid_ = eng::timer::once(std::chrono::seconds(1), [this]
//     {
//         update_internal_state();
//     });
// }
//
// void frequency_converter::update_internal_state()
// {
//     // Запрашиваем текущее состояние
//     modbus_rtu_slave::read_holding(0xA411, 3,
//         &frequency_converter::read_status_done, &frequency_converter::read_device_offline);
// }
//
// // Прочитано 3 регистра
// void frequency_converter::read_status_done(std::span<std::uint16_t const> regs)
// {
//     // Если ошибка
//     if (regs[0] == 2)
//     {
//         std::uint32_t emask;
//         std::memcpy(&emask, regs.data() + 1, sizeof(emask));
//
//         update_error_state(emask);
//     }
//     else
//     {
//         update_normal_state(regs[0] == 1);
//     }
//
//     modbus_rtu_slave::read_holding(0xA430, 1,
//         &frequency_converter::read_F_done, &frequency_converter::read_device_offline);
// }
//
// // Прочитан 1 регистр
// void frequency_converter::read_F_done(std::span<std::uint16_t const> regs)
// {
//     F_ = regs[0] * 10;
//
//     modbus_rtu_slave::read_holding(0xA432, 3,
//         &frequency_converter::read_UI_done, &frequency_converter::read_device_offline);
// }
//
// // Прочитано 3 регистра
// void frequency_converter::read_UI_done(std::span<std::uint16_t const> regs)
// {
//     U_in_ = regs[0] * 1.0;
//     U_out_ = regs[1] * 1.0;
//     I_ = regs[2] * 0.1;
//
//     modbus_rtu_slave::read_holding(0xA437, 1,
//         &frequency_converter::read_P_done, &frequency_converter::read_device_offline);
// }
//
// // Прочитан 1 регистр
// void frequency_converter::read_P_done(std::span<std::uint16_t const> regs)
// {
//     P_ = regs[0] * 100;
//
//     node::set_port_value(port_measure_, { P_, U_in_, U_out_, I_, F_ });
//
//     init_next_read();
// }
//
// // [offline]
// void frequency_converter::set_offline_state()
// {
//     if (offline_) return;
//
//     offline_ = true;
//
//     node::set_port_value(port_state_, { false });
//     eng::log::info("state: online = false");
//
//     node::set_port_value(port_damage_, { true });
//     eng::log::info("damage: true");
//
//     if (powered_)
//     {
//         node::set_port_value(port_powered_, { false });
//         eng::log::info("powered: false");
//         powered_ = false;
//     }
// }
//
// // [online, error, emask]
// void frequency_converter::update_error_state(std::uint32_t emask)
// {
//     if (!offline_ && inerror_ && emask_ == emask)
//         return;
//
//     if (offline_ || !inerror_)
//     {
//         node::set_port_value(port_damage_, { true });
//         eng::log::info("damage: true");
//     }
//
//     if (powered_)
//     {
//         node::set_port_value(port_powered_, { false });
//         eng::log::info("powered: false");
//         powered_ = false;
//     }
//
//     offline_ = false;
//     inerror_ = true;
//     emask_ = emask;
//
//     node::set_port_value(port_state_, { true, true, emask });
//     eng::log::info("state: online: true, error: true, emask: {}", emask);
// }
//
// // [online, ok, powered]
// void frequency_converter::update_normal_state(bool powered)
// {
//     if (!offline_ && !inerror_ && powered_ == powered)
//         return;
//
//     if (offline_ || inerror_)
//     {
//         node::set_port_value(port_damage_, { false });
//         eng::log::info("damage: false");
//     }
//
//     offline_ = false;
//     inerror_ = false;
//     powered_ = powered;
//
//     node::set_port_value(port_state_, { true, false, powered });
//     eng::log::info("state: online: true, error: false, powered: {}", powered);
// }
//
//
