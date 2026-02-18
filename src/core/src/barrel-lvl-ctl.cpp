#include "barrel-lvl-ctl.hpp"

#include <eng/log.hpp>

// Данный модуль поддерживает температуру в заданном конфигурацией диапазоне
// И занимается доливкой жидкости в вверенную ему емкость

barrel_lvl_ctl::barrel_lvl_ctl(std::string_view key)
    : eng::sibus::node(std::format("barrel-lvl-ctl-{}", key))
{
    // Текущее значение протока
    node::add_input_port_unsafe("FC", [this](eng::abc::pack args)
    {
        update_flow_rate(args ? eng::abc::get<double>(args) : NAN);
    });

    ictl_ = node::add_input_wire();

    // Команда на доливку определенного количества жидкости
    node::set_activate_handler(ictl_, [this](eng::abc::pack args)
    {
        start_refilling(eng::abc::get<double>(args));
    });

    // Прекращение процесса доливки
    node::set_deactivate_handler(ictl_, [this]
    {
        stop_refilling();
    });

    valve_ = node::add_output_port("valve");

    // Создаем таймер для гарантированного
    // обновления значения раз в 100 мс
    tid_ = eng::timer::create([this]
    {
        decrease_amount(flow_rate_);
    });
}

void barrel_lvl_ctl::start_refilling(double value)
{
    // Инициируем процесс доливки, для этого:
    //  - открываем клапан
    //  - запускаем таймер

    target_volume_ = value;

    // Если значение меняется в процессе доливки, просто обновляем
    // цель не сбрасывая количество уже долитой жидкости
    if (eng::timer::is_running(tid_))
    {
        decrease_amount(flow_rate_);
        return;
    }

    refilled_volume_ = 0.0;

    eng::timer::start(tid_, std::chrono::milliseconds(100));
    stopwatch_.start();

    node::set_port_value(valve_, { true });
}

void barrel_lvl_ctl::stop_refilling()
{
    node::set_port_value(valve_, { false });
    eng::timer::stop(tid_);
}

void barrel_lvl_ctl::update_flow_rate(double value)
{
    if (eng::timer::is_running(tid_))
    {
        double middle = (flow_rate_ + value) * 0.5;

        if (std::isnan(middle))
        {
            stop_refilling();

            eng::log::error("{}: Отсутствуют данные по протоку", name());
            node::terminate(ictl_, "Отсутствуют данные по протоку");
        }
        else
        {
            decrease_amount(middle);
        }
    }
    else
    {
        if (is_flow_sensor_first_init(value))
            node::ready(ictl_);
    }

    flow_rate_ = value;
}

void barrel_lvl_ctl::decrease_amount(double flow_rate)
{
    auto dt = stopwatch_.elapsed_seconds<double>();
    stopwatch_.restart();

    double inlet = dt * (flow_rate / 60.0);

    refilled_volume_ += inlet;

    if (refilled_volume_ < target_volume_)
        return;

    stop_refilling();

    node::ready(ictl_);
}

bool barrel_lvl_ctl::is_flow_sensor_first_init(double value) const noexcept
{
    return std::isnan(flow_rate_) && !std::isnan(value);
}

