#include "stuff-ctl.hpp"
#include "common/load-axis-list.hpp"

#include <eng/log.hpp>

#include <algorithm>

stuff_ctl::stuff_ctl()
    : eng::sibus::node("stuff-ctl")
{
    ictl_ = node::add_input_wire();

    node::set_activate_handler(ictl_, [this](eng::abc::pack args)
    {
        start_command_execution(std::move(args));
    });
    node::set_deactivate_handler(ictl_, [this]
    {
        if (!in_use_any())
            return;
        deactivate_all();
    });

    fc_.ctl = node::add_output_wire("fc");
    node::set_wire_status_handler(fc_.ctl, [this](eng::sibus::istatus, std::string_view emsg)
    {
        fc_.emsg = emsg;
        ctl_fc_state(emsg);
    });

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        sp_[i].ctl = node::add_output_wire(std::format("sp{}", i));
        node::set_wire_status_handler(sp_[i].ctl, [this, i](eng::sibus::istatus, std::string_view emsg)
        {
            sp_[i].emsg = emsg;
            ctl_sp_state(i, emsg);
        });
    }

    axis_load_ok_ = ambit::load_axis_list([this](char axis, std::string_view axis_name, bool rotation)
    {
        if (!rotation) return;
        axis_[axis] = { false };
        axis_name_[axis] = axis_name;
    });

    for (auto &[ axis, desc ] : axis_)
    {
        desc.ctl = node::add_output_wire(std::format("{}", axis));
        node::set_wire_status_handler(desc.ctl, [this, axis](eng::sibus::istatus, std::string_view emsg)
        {
            axis_[axis].emsg = emsg;
            ctl_axis_state(axis, emsg);
        });
    }
}

void stuff_ctl::terminate_execution(std::string_view name, std::string_view emsg)
{
    node::ready(ictl_, std::format("{}: {}", name, emsg));
    deactivate_all();
}

void stuff_ctl::deactivate_all()
{
    if (fc_.in_active)
    {
        fc_.in_active = false;
        node::deactivate(fc_.ctl);
    }
    fc_.in_use = false;

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        if (sp_[i].in_active)
        {
            sp_[i].in_active = false;
            node::deactivate(sp_[i].ctl);
        }
        sp_[i].in_use = false;
    }

    for (auto &[ axis, desc ] : axis_)
    {
        if (desc.in_active)
        {
            desc.in_active = false;
            node::activate(desc.ctl, { "spin", 0.0 });
        }
        desc.in_use = false;
    }
}

void stuff_ctl::start_command_execution(eng::abc::pack args)
{
    static std::unordered_map<std::string_view, commands_handler> const map {
        { "prepare",    &stuff_ctl::cmd_prepare     },
        { "operation",  &stuff_ctl::cmd_operation   },
    };

    std::string_view cmd = eng::abc::get<std::string_view>(args, 0);
    auto it = map.find(cmd);
    if (it == map.end())
    {
        node::reject(ictl_, std::format("Незнакомая комманда: {}", cmd));
        return;
    }

    args.pop_front();

    (this->*(it->second))(args);
}

// Нам сообщают, какие устройства задействованы в режиме
// [ fc, sp0, sp1, sp2, N, [ X, Y, ... ] ]
void stuff_ctl::cmd_prepare(eng::abc::pack const &args)
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
        axis_[axis].in_use = true;
        eng::log::info("{}: init: {} = {}", name(), axis, axis_[axis].in_use);
    }

    // Проверяем готовность системы
    if (fc_.in_use && !fc_.emsg.empty())
    {
        eng::log::error("{}: {}", name(), fc_.emsg);
        node::reject(ictl_, fc_.emsg);
        return;
    }

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        if (sp_[i].in_use && !sp_[i].emsg.empty())
        {
            eng::log::error("{}: Спрейер №{} {}", name(), i + 1, sp_[i].emsg);
            node::reject(ictl_, std::format("Спрейер №{} {}", i + 1, sp_[i].emsg));
            return;
        }
    }

    for (auto const &[ axis, desc ] : axis_)
    {
        if (desc.in_use && !desc.emsg.empty())
        {
            eng::log::error("{}: Ось {} недоступна для работы: {}", name(), axis, desc.emsg);
            node::reject(ictl_, std::format("Ось {}: {}", axis, desc.emsg));
            return;
        }
    }
}

// Разбираем операцию и передаем на выполнение
// [ fc-i, fc-p, sp0, sp1, sp2, N, [ axis, speed, ... ] ]
void stuff_ctl::cmd_operation(eng::abc::pack const &args)
{
    std::size_t iarg = 0;

    double fc_i = eng::abc::get<double>(args, iarg++);
    double fc_p = eng::abc::get<double>(args, iarg++);
    if (fc_.in_use) apply_fc_state(fc_i, fc_p);

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        bool active = eng::abc::get<bool>(args, iarg++);
        if (sp_[i].in_use) apply_sp_state(i, active);
    }

    std::size_t axis_count = eng::abc::get<std::uint8_t>(args, iarg++);
    for (std::size_t i = 0; i < axis_count; ++i)
    {
        char axis = eng::abc::get<char>(args, iarg++);
        double speed = eng::abc::get<double>(args, iarg++);
        if (axis_[axis].in_use) apply_axis_state(axis, speed);
    }
}

void stuff_ctl::apply_fc_state(double i, double p)
{
    eng::log::info("{}: FC-I = {}", name(), i);
    eng::log::info("{}: FC-P = {}", name(), p);

    fc_.in_active = (p != 0.0);
    if (fc_.in_active)
        node::activate(fc_.ctl, { i, p });
    else
        node::deactivate(fc_.ctl);
}

void stuff_ctl::apply_sp_state(std::size_t idx, bool active)
{
    eng::log::info("{}: SP{} = {}", name(), idx, active);

    auto &sp = sp_[idx];
    sp.in_active = active;

    if (sp.in_active)
        node::activate(sp.ctl, { });
    else
        node::deactivate(sp.ctl);
}

void stuff_ctl::apply_axis_state(char key, double speed)
{
    eng::log::info("{}: {} = {}", name(), key, speed);

    auto &axis = axis_[key];
    axis.in_active = (speed != 0.0);

    node::activate(axis.ctl, { "spin", speed });
}

void stuff_ctl::ctl_fc_state(std::string_view emsg)
{
    // Мы не используем ПЧ
    if (!fc_.in_use) return;

    if (emsg.empty())
    {
        if (fc_.in_active == node::is_active(fc_.ctl))
            return;
        emsg = "Не соответствует заданному состоянию";
    }

    terminate_execution("ПЧ", emsg);
}

void stuff_ctl::ctl_sp_state(std::size_t idx, std::string_view emsg)
{
    auto &sp = sp_[idx];

    if (!sp.in_use) return;

    if (emsg.empty())
    {
        if (sp.in_active == node::is_active(sp.ctl))
            return;
        emsg = "Не соответствует заданному состоянию";
    }

    terminate_execution(std::format("Спрейер №{}", idx + 1), emsg);
}

void stuff_ctl::ctl_axis_state(char key, std::string_view emsg)
{
    auto &axis = axis_[key];

    if (!axis.in_use) return;

    if (emsg.empty())
    {
        if (axis.in_active == node::is_active(axis.ctl))
            return;
        emsg = "Не соответствует заданному состоянию";
    }

    terminate_execution(axis_name_[key], emsg);
}

bool stuff_ctl::in_use_any() const
{
    if (fc_.in_use)
        return true;

    for (std::size_t i = 0; i < sp_.size(); ++i)
        if (sp_[i].in_use) return true;

    for (auto const &[ _, desc ] : axis_)
        if (desc.in_use) return true;

    return false;
}

void stuff_ctl::register_on_bus_done()
{
    if (axis_load_ok_)
        node::ready(ictl_);
}

