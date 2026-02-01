#include "stuff-ctl.hpp"
#include "eng/sibus/sibus.hpp"

#include <eng/log.hpp>
#include <eng/json.hpp>

#include <filesystem>
#include <algorithm>

stuff_ctl::stuff_ctl()
    : eng::sibus::node("stuff-ctl")
    , state_(&stuff_ctl::s_ready_to_work)
{
    ictl_ = node::add_input_wire();

    node::set_activate_handler(ictl_, [this](eng::abc::pack args)
    {
        eng::log::info("{}: ACTIVATE", name());
        activate(std::move(args));
    });
    node::set_deactivate_handler(ictl_, [this]
    {
        eng::log::info("{}: DEACTIVATE", name());
        deactivate();
    });
    node::set_wire_request_handler(ictl_, [this](eng::abc::pack args)
    {
        eng::log::info("{}: do-command: {}", name(), eng::abc::get_sv(args));
        do_command(std::move(args));
    });

    fc_.ctl = node::add_output_wire("fc");
    node::set_wire_status_handler(fc_.ctl, [this] {
        (this->*state_)();
    });

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        sp_[i].ctl = node::add_output_wire(std::format("sp{}", i));
        node::set_wire_status_handler(sp_[i].ctl, [this, i] {
            (this->*state_)();
        });
    }

    load_axis_list();

    for (auto &[ axis, desc ] : axis_)
    {
        desc.ctl = node::add_output_wire(std::format("{}", axis));
        node::set_wire_status_handler(desc.ctl, [this, axis] {
            (this->*state_)();
        });
    }
}

// Нам сообщают, какие устройства задействованы в режиме
// [ fc, sp0, sp1, sp2, N, [ X, Y, ... ] ]
void stuff_ctl::activate(eng::abc::pack args)
{
    std::size_t iarg = 0;

    fc_.in_use = eng::abc::get<bool>(args, iarg++);
    eng::log::info("{}: init: fc = {}", name(), fc_.in_use);

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        sp_[i].in_use = eng::abc::get<bool>(args, iarg++);
        eng::log::info("{}: init: sp{} = {}", name(), i, sp_[i].in_use);
    }

    // Сбрасываем предидущее состояние
    std::ranges::for_each(axis_, [](auto &pair)
        { pair.second.in_use = false; });

    std::size_t axis_count = eng::abc::get<std::uint8_t>(args, iarg++);
    for (std::size_t i = 0; i < axis_count; ++i)
    {
        char axis = eng::abc::get<char>(args, iarg++);

        if (!axis_.contains(axis))
        {
            eng::log::error("{}: Требуемая ось {} отсутствует в конфигурации системы", name(), axis);
            node::deactivated(ictl_);
            return;
        }

        axis_[axis].in_use = true;
        eng::log::info("{}: init: {} = {}", name(), axis, axis_[axis].in_use);
    }

    // Теперь нам надо убедиться что они доступны
    if (!is_stuff_usable())
    {
        eng::log::error("{}: Система не готова к выполнению режима", name());
        node::deactivated(ictl_);
        return;
    }

    eng::log::info("{}: Система готова к выполнению режима", name());

    node::activated(ictl_);

    state_ = &stuff_ctl::s_in_proccess;
}

void stuff_ctl::deactivate()
{
    state_ = &stuff_ctl::s_deactivating;
    s_deactivating();
}

void stuff_ctl::do_command(eng::abc::pack args)
{
    if (state_ != &stuff_ctl::s_in_proccess)
    {
        node::wire_response(ictl_, false, { std::format("Система не готова выполнять команды") });
        return;
    }

    static std::unordered_map<std::string_view, activate_command> const map {
        { "operation", &stuff_ctl::cmd_operation   },
    };

    std::string_view cmd = eng::abc::get_sv(args);

    auto it = map.find(cmd);
    if (it == map.end())
    {
        node::wire_response(ictl_, false, { std::format("Незнакомая комманда: {}", cmd) });
        return;
    }

    args.pop_front();

    (this->*(it->second))(std::move(args));

    (this->*state_)();
}

// Разбираем операцию и передаем на выполнение
// [ fc-i, fc-p, sp0, sp1, sp2, N, [ axis, speed, ... ] ]
void stuff_ctl::cmd_operation(eng::abc::pack args)
{
    std::size_t iarg = 0;

    double fc_i = eng::abc::get<double>(args, iarg++);
    double fc_p = eng::abc::get<double>(args, iarg++);
    apply_fc_state(fc_i, fc_p);

    for (std::size_t i = 0; i < sp_.size(); ++i)
        apply_sp_state(i, eng::abc::get<bool>(args, iarg++));

    std::size_t axis_count = eng::abc::get<std::uint8_t>(args, iarg++);
    for (std::size_t i = 0; i < axis_count; ++i)
    {
        char axis = eng::abc::get<char>(args, iarg++);
        double speed = eng::abc::get<double>(args, iarg++);
        apply_axis_state(axis, speed);
    }

    node::wire_response(ictl_, true, { });
}

void stuff_ctl::s_ready_to_work()
{
}

void stuff_ctl::s_in_proccess()
{
    bool all_ok = true;

    // all_ok &= is_state_correct(fc_);
    //
    // for (std::size_t i = 0; i < sp_.size(); ++i)
    //     all_ok &= is_state_correct(sp_[i]);
    //
    // for (auto &[ _, desc ] : axis_)
    //     all_ok &= is_state_correct(desc);

    if (all_ok) return;

    eng::log::error("{}: Устройства не прошли проверку состояния", name());

    deactivate();
}

// Приводим все устройства в штатное состояние
void stuff_ctl::s_deactivating()
{
    bool all_done = true;

    all_done &= is_deactivated(fc_);

    for (std::size_t i = 0; i < sp_.size(); ++i)
        all_done &= is_deactivated(sp_[i]);

    for (auto &[ _, desc ] : axis_)
        all_done &= is_deactivated(desc);

    if (!all_done) return;

    node::deactivated(ictl_);
    state_ = &stuff_ctl::s_ready_to_work;
}

bool stuff_ctl::is_deactivated(unit_t &unit)
{
    if (!unit.in_use)
        return true;

    if (!node::is_allow(unit.ctl))
    {
        unit.in_use = false;
        return true;
    }

    if (node::is_transiting(unit.ctl))
        return false;

    if (node::is_active(unit.ctl))
    {
        node::deactivate(unit.ctl);
        return false;
    }

    if (node::is_ready(unit.ctl))
    {
        unit.in_use = false;
        return true; 
    }

    return false;
}

bool stuff_ctl::is_state_correct(unit_t &unit)
{
    if (!unit.in_use)
        return true;

    if (!node::is_allow(unit.ctl))
        return false;

    if (node::is_transiting(unit.ctl))
        return true;

    return node::is_active(unit.ctl) == unit.active;
}

void stuff_ctl::apply_fc_state(double i, double p)
{
    if (!fc_.in_use) return;

    eng::log::info("{}: FC-I = {}", name(), i);
    eng::log::info("{}: FC-P = {}", name(), p);

    fc_.active = p != 0.0;
    if (fc_.active)
        node::activate(fc_.ctl, { i, p });
    else
        node::deactivate(fc_.ctl);
}

void stuff_ctl::apply_sp_state(std::size_t idx, bool value)
{
    if (!sp_[idx].in_use) return;

    eng::log::info("{}: SP{} = {}", name(), idx, value);

    sp_[idx].active = value;
    if (value)
        node::activate(sp_[idx].ctl, { });
    else
        node::deactivate(sp_[idx].ctl);
}

void stuff_ctl::apply_axis_state(char axis, double speed)
{
    if (!axis_[axis].in_use) return;

    axis_[axis].active = speed != 0.0;
    if (axis_[axis].active)
        node::activate(axis_[axis].ctl, { speed });
    else
        node::deactivate(axis_[axis].ctl);
}

bool stuff_ctl::is_stuff_usable() const
{
    bool result = true;

    if (fc_.in_use && !node::is_allow(fc_.ctl))
    {
        eng::log::error("{}: ПЧ недоступен для работы", name());
        result = false;
    }

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        if (sp_[i].in_use && !node::is_allow(sp_[i].ctl))
        {
            eng::log::error("{}: Спрейер №{} недоступен для работы", name(), i);
            result = false;
        }
    }

    for (auto const &[ axis, desc ] : axis_)
    {
        if (desc.in_use && !node::is_allow(desc.ctl))
        {
            eng::log::error("{}: Ось {} недоступна для работы", name(), axis);
            result = false;
        }
    }

    return result;
}

void stuff_ctl::load_axis_list()
{
    char const *LIAEM_RO_PATH = std::getenv("LIAEM_RO_PATH");
    if (LIAEM_RO_PATH == nullptr)
    {
        eng::log::error("{}: LIAEM_RO_PATH not set", name());
        return;
    }

    std::filesystem::path path(LIAEM_RO_PATH);
    path /= "axis.json";

    try
    {
        eng::json::array cfg(path);
        cfg.for_each([this](std::size_t, eng::json::value v)
        {
            eng::json::object obj(v);
            char axis = obj.get_sv("axis")[0];
            axis_[axis] = { false };
        });
    }
    catch(std::exception const &e)
    {
        eng::log::error("{}: {}", name(), e.what());
        return;
    }
}

