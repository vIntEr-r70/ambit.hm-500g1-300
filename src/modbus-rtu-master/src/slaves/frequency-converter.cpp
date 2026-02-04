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
    node::set_activate_handler(ictl_, [this](eng::abc::pack args)
    {
        activate(std::move(args));
    });
    node::set_deactivate_handler(ictl_, [this]
    {
        deactivate();
    });

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

void frequency_converter::register_on_bus_done()
{
    node::set_ready(ictl_);
}

/////////////////////////////////////////////////////////////////////////////////////////////

// Запускаем устройство в работу
void frequency_converter::activate(eng::abc::pack args)
{
    eng::log::info("{}: {}", name(), __func__);

    if (!is_online())
    {
        node::terminate(ictl_, "Отсутствует связь с устройством");
        return;
    }

    if (hw_state_ == &frequency_converter::s_damaged)
    {
        node::terminate(ictl_, "Устройство в состоянии аварии");
        return;
    }

    if (args.size() == 2)
    {
        iup_[0] = eng::abc::get<double>(args, 0);
        iup_[2] = eng::abc::get<double>(args, 1);
    }

    // Записываем уставки
    std::array<std::uint16_t, 3> iup{
        static_cast<std::uint16_t>(std::lround(iup_[0] * 10.0f)),
        static_cast<std::uint16_t>(std::lround(iup_[1])),
        static_cast<std::uint16_t>(std::lround(iup_[2] * 0.01f))
    };
    modbus_unit::write_multiple(0xA420, { iup.data(), iup.size() });

    if (hw_state_ == &frequency_converter::s_powered)
        return;

    // Передаем команду на запуск
    write_task_handler_ = {
        modbus_unit::write_single(0xA410, 0x0001),
        &frequency_converter::w_start
    };
}

// Останавливаем устройство
void frequency_converter::deactivate()
{
    eng::log::info("{}: {}", name(), __func__);

    if (!is_online())
    {
        node::set_ready(ictl_);
        return;
    }

    if (hw_state_ == &frequency_converter::s_idle)
    {
        node::set_ready(ictl_);
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

    node::set_ready(ictl_);
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
        node::set_ready(ictl_);
        return;
    case 1:
        return;
    }

    node::set_ready(ictl_);

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
    eng::log::info("{}: {}", name(), __func__);

    node::set_ready(ictl_);

    eng::log::info("frequency_converter::now_unit_online");

    modbus_unit::write_single(0xA410, 0x0002);
    node::set_port_value(p_out_[pout::online], { true });
}

void frequency_converter::connection_was_lost()
{
    eng::log::info("{}: {}", name(), __func__);

    if (hw_state_ == &frequency_converter::s_powered)
        node::terminate(ictl_, "Связь с устройством потеряна");

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
    eng::log::info("{}: {}", name(), __func__);

    // Если логика в режиме ожидания завершения операции
    if (write_task_handler_.handler && write_task_handler_.idx == idx)
    {
        (this->*write_task_handler_.handler)(true);
        write_task_handler_.handler = nullptr;
    }
}

void frequency_converter::w_start(bool success)
{
    eng::log::info("{}: {}", name(), __func__);

    if (!success)
        node::terminate(ictl_, "Не удалось включить устройство");
}

void frequency_converter::w_stop(bool success)
{
    eng::log::info("{}: {}", name(), __func__);

    if (!success)
        node::terminate(ictl_, "Не удалось выключить устройство");
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

