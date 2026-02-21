#include "frequency-converter.hpp"
#include "modbus-unit.hpp"

#include <eng/timer.hpp>
#include <eng/log.hpp>

frequency_converter::frequency_converter(std::size_t unit_id)
    : eng::sibus::node{ "fc-ctl" }
    , modbus_unit(unit_id)
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

    node::add_input_port("limits", [this](eng::abc::pack args)
    {
        I_max_ = eng::abc::get<double>(args, 1);
        iup_[kU] = eng::abc::get<double>(args, 2);
        P_max_ = I_max_ * iup_[kU];

        iup_[kI] = std::min(I_max_, iup_[kI]);
        iup_[kP] = std::min(P_max_, iup_[kP]);

        write_sets();
    });

    node::add_input_port("I", [this](eng::abc::pack args)
    {
        iup_[kI] = std::min(I_max_, eng::abc::get<double>(args));
        write_sets();
    });

    node::add_input_port("P", [this](eng::abc::pack args)
    {
        iup_[kP] = std::min(P_max_, eng::abc::get<double>(args));
        write_sets();
    });
}

void frequency_converter::register_on_bus_done()
{
    // Опрос текущего состояния устройства
    std::size_t idx = modbus_unit::add_read_task(0xA411, 5, 300);
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

    modbus_unit::start_working();
}

/////////////////////////////////////////////////////////////////////////////////////////////

// Запускаем устройство в работу
void frequency_converter::activate(eng::abc::pack args)
{
    eng::log::info("{}: {}", name(), __func__);

    if (args.size() == 2)
    {
        // Новые уставки
        priority_sets_ = {
                .I = std::min(I_max_, eng::abc::get<double>(args, 0)),
                .P = std::min(P_max_, eng::abc::get<double>(args, 1))
            };
        write_priority_sets();
    }
    else
    {
        priority_sets_.reset();
        write_sets();
    }

    // Если еще не запущены, передаем команду на запуск
    if (status_.value() != estatus::powered)
    {
        eng::log::info("{}: CMD: write start and reset errors", name());
        modbus_unit::write_single(0xA410, 0x0003);
    }
}

// Останавливаем устройство
void frequency_converter::deactivate()
{
    eng::log::info("{}: {}", name(), __func__);

    switch (status_.value())
    {
    case estatus::idle:
        break;

    case estatus::powered:
        eng::log::info("{}: CMD: write stop", name());
        modbus_unit::write_single(0xA410, 0x0000);
        break;

    case estatus::damaged:
        eng::log::info("{}: CMD: reset errors", name());
        modbus_unit::write_single(0xA410, 0x0002);
        break;
    }
}

void frequency_converter::now_unit_online()
{
    eng::log::info("{}: {}", name(), __func__);

    // Отправляем команду на сброс ошибок
    eng::log::info("{}: CMD: reset errors", name());
    modbus_unit::write_single(0xA410, 0x0002);

    node::set_port_value(p_out_[pout::online], { true });
}

void frequency_converter::connection_was_lost()
{
    eng::log::info("{}: {}", name(), __func__);

    node::terminate(ictl_, "Связь с устройством потеряна");
    status_.reset();

    // Мы не знаем какое состояние теперь у ПЧ
    std::ranges::fill(damages_, 0);

    node::set_port_value(p_out_[pout::powered], { false });
    node::set_port_value(p_out_[pout::damaged], { false });

    node::set_port_value(p_out_[pout::F], { });
    node::set_port_value(p_out_[pout::U_in], { });
    node::set_port_value(p_out_[pout::U_out], { });
    node::set_port_value(p_out_[pout::I], { });
    node::set_port_value(p_out_[pout::P], { });

    node::set_port_value(p_out_[pout::online], { false });
}

/////////////////////////////////////////////////////////////////////////////////////////////

void frequency_converter::read_task_done(std::size_t idx, readed_regs_t regs)
{
    (this->*read_task_handlers_[idx])(regs);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void frequency_converter::read_status_done(readed_regs_t regs)
{
    estatus status = static_cast<estatus>(regs[0]);

    // Если статус не авария, достаточно проверить его изменение
    if (status != estatus::damaged)
    {
        // Значение статуса не изменилось
        if (status == status_ && status_.has_value())
            return;
    }

    // Мы выключились либо была сброшена авария
    if (status == estatus::idle)
    {
        eng::log::info("{}: status = idle", name());

        node::set_port_value(p_out_[pout::damaged], { false });
        node::set_port_value(p_out_[pout::powered], { false });
    }
    // Мы включились
    else if (status == estatus::powered)
    {
        eng::log::info("{}: status = powered", name());

        node::set_port_value(p_out_[pout::powered], { true });
    }
    // Мы оказались в аварии
    else if (status == estatus::damaged)
    {
        // Если статус авария, могут измениться маски ошибок

        auto src = regs.subspan(1);
        if (status == status_ && status_.has_value() && std::ranges::equal(src, damages_))
            return;

        eng::log::info("{}: status = damaged", name());

        // Запоминаем новые значения масок ошибок
        std::ranges::copy(src, damages_.begin());

        node::set_port_value(p_out_[pout::damaged],
            { true, damages_[0], damages_[1], damages_[2], damages_[3] });
    }
    else
    {
        eng::log::info("{}: status = ? ({})", name(), regs[0]);
        return;
    }

    // Если это первый статус с момента восстановления связи
    if (!status_.has_value() || (status_.value() != estatus::powered && node::is_active(ictl_)))
        node::ready(ictl_);

    // Запоминаем новое значение статуса
    status_ = status;
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

void frequency_converter::write_priority_sets()
{
    auto const &sets = priority_sets_.value();

    // Записываем уставки
    std::array<std::uint16_t, 3> iup{
        static_cast<std::uint16_t>(std::lround(sets.I * 10.0)),
        static_cast<std::uint16_t>(std::lround(iup_[kU])),
        static_cast<std::uint16_t>(std::lround(sets.P * 0.01))
    };

    eng::log::info("{}: I = {}, U = {}, P = {}", name(), iup[kI], iup[kU], iup[kP]);

    eng::log::info("{}: CMD: write sets", name());
    modbus_unit::write_multiple(0xA420, { iup.data(), iup.size() });
}

void frequency_converter::write_sets()
{
    // Игнорируем изменение уставок если устройство не в сети
    if (!is_online() || priority_sets_.has_value()) return;

    // Записываем уставки
    std::array<std::uint16_t, 3> iup{
        static_cast<std::uint16_t>(std::lround(iup_[kI] * 10.0)),
        static_cast<std::uint16_t>(std::lround(iup_[kU])),
        static_cast<std::uint16_t>(std::lround(iup_[kP] * 0.01))
    };

    eng::log::info("{}: I = {}, U = {}, P = {}", name(), iup[kI], iup[kU], iup[kP]);

    eng::log::info("{}: CMD: write sets", name());
    modbus_unit::write_multiple(0xA420, { iup.data(), iup.size() });
}


