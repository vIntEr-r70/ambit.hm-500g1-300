#include "stuff-ctl.hpp"
#include "eng/sibus/sibus.hpp"

#include <eng/log.hpp>
#include <eng/json.hpp>

#include <filesystem>
#include <algorithm>

stuff_ctl::stuff_ctl()
    : eng::sibus::node("stuff-ctl")
    , isc_(this)
{
    ictl_ = node::add_input_wire();

    node::set_activate_handler(ictl_, [this](eng::abc::pack args)
    {
        activate(std::move(args));
    });
    node::set_deactivate_handler(ictl_, [this]
    {
        deactivate();
    });

    // node::set_wire_request_handler(ictl_, [this](eng::abc::pack args)
    // {
    //     eng::log::info("{}: do-command: {}", name(), eng::abc::get_sv(args));
    //     do_command(std::move(args));
    //     isc_.touch_current_state();
    // });

    fc_.ctl = node::add_output_wire("fc");
    node::set_wire_status_handler(fc_.ctl, [this] {
        isc_.touch_current_state();
    });

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        sp_[i].ctl = node::add_output_wire(std::format("sp{}", i));
        node::set_wire_status_handler(sp_[i].ctl, [this, i] {
            isc_.touch_current_state();
        });
    }

    load_axis_list();

    for (auto &[ axis, desc ] : axis_)
    {
        desc.ctl = node::add_output_wire(std::format("{}", axis));
        node::set_wire_status_handler(desc.ctl, [this, axis] {
            isc_.touch_current_state();
        });
    }
}

// Вызывается только если в состоянии ready или active

// Нам сообщают, какие устройства задействованы в режиме
// [ fc, sp0, sp1, sp2, N, [ X, Y, ... ] ]
void stuff_ctl::activate(eng::abc::pack args)
{
    // Если мы уже активированы, повторная активация не допускается
    if (!isc_.is_in_state(nullptr))
    {
        eng::log::info("{}: do-command: {}", name(), eng::abc::get_sv(args));
        do_command(std::move(args));
        isc_.touch_current_state();
        // node::reject(ictl_, "Система уже активирована");
        return;
    }

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
    if (!is_stuff_usable())
    {
        node::reject(ictl_, "Система не готова к выполнению режима");
        return;
    }

    isc_.switch_to_state(in_use_any() ?
        &stuff_ctl::s_ctl_state : &stuff_ctl::s_lazy_state);
}

// Может быть 
void stuff_ctl::deactivate()
{
    // Деактивируем все с чем работали

    if (fc_.in_use && !node::is_ready(fc_.ctl))
        node::deactivate(fc_.ctl);

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        if (sp_[i].in_use && !node::is_ready(sp_[i].ctl))
            node::deactivate(sp_[i].ctl);
    }

    for (auto const &[ axis, desc ] : axis_)
    {
        if (desc.in_use && !node::is_ready(desc.ctl))
            node::deactivate(desc.ctl);
    }

    node::terminate(ictl_, "Приведение системы в штатное состояние");

    isc_.switch_to_state(&stuff_ctl::s_wait_system_ready);
}

void stuff_ctl::s_wait_system_ready()
{
    if (!is_stuff_usable())
        return;

    node::set_ready(ictl_);
    isc_.switch_to_state(nullptr);
}

// Мы активны но задач нам приходить не будет
void stuff_ctl::s_lazy_state()
{
    eng::log::info("{}: {}", name(), __func__);
}

// Мы активны и должны управлять периферией
void stuff_ctl::s_ctl_state()
{
    eng::log::info("{}: {}", name(), __func__);

    apply_fc_state();

    for (std::size_t i = 0; i < sp_.size(); ++i)
        apply_sp_state(i);

    for (auto &[ axis, _ ] : axis_)
        apply_axis_state(axis);
}

void stuff_ctl::do_command(eng::abc::pack args)
{
    static std::unordered_map<std::string_view, activate_command> const map {
        { "operation", &stuff_ctl::cmd_operation   },
    };

    std::string_view cmd = eng::abc::get_sv(args);

    auto it = map.find(cmd);
    if (it == map.end())
    {
        node::reject(ictl_, std::format("Незнакомая комманда: {}", cmd));
        // node::wire_response(ictl_, false, { std::format("Незнакомая комманда: {}", cmd) });
        return;
    }

    args.pop_front();

    (this->*(it->second))(std::move(args));
}

// Разбираем операцию и передаем на выполнение
// [ fc-i, fc-p, sp0, sp1, sp2, N, [ axis, speed, ... ] ]
void stuff_ctl::cmd_operation(eng::abc::pack args)
{
    std::size_t iarg = 0;

    double fc_i = eng::abc::get<double>(args, iarg++);
    double fc_p = eng::abc::get<double>(args, iarg++);
    fc_.value = { .i = fc_i, .p = fc_p };

    for (std::size_t i = 0; i < sp_.size(); ++i)
        sp_[i].value = eng::abc::get<bool>(args, iarg++);

    std::size_t axis_count = eng::abc::get<std::uint8_t>(args, iarg++);
    for (std::size_t i = 0; i < axis_count; ++i)
    {
        char axis = eng::abc::get<char>(args, iarg++);
        axis_[axis].value = eng::abc::get<double>(args, iarg++);
    }

    // node::wire_response(ictl_, true, { });
}

void stuff_ctl::apply_fc_state()
{
    if (!fc_.in_use || !fc_.value.has_value())
        return;

    if (node::is_transiting(fc_.ctl))
        return;

    auto v = *fc_.value;

    eng::log::info("{}: FC-I = {}", name(), v.i);
    eng::log::info("{}: FC-P = {}", name(), v.p);

#ifdef BUILDROOT
    if (v.p != 0.0)
    {
        if (node::is_ready(fc_.ctl) || node::is_active(fc_.ctl))
            node::activate(fc_.ctl, { v.i, v.p });
    }
    else
    {
        if (node::is_active(fc_.ctl))
            node::deactivate(fc_.ctl);
    }
#endif

    fc_.value.reset();
}

void stuff_ctl::apply_sp_state(std::size_t idx)
{
    auto &sp = sp_[idx];

    if (!sp.in_use || !sp.value.has_value())
        return;

    if (node::is_transiting(sp.ctl))
        return;

    eng::log::info("{}: SP{} = {}", name(), idx, *sp.value);

#ifdef BUILDROOT
    if (*sp.value)
    {
        if (node::is_ready(sp.ctl))
            node::activate(sp.ctl, { });
    }
    else
    {
        if (node::is_active(sp.ctl))
            node::deactivate(sp.ctl);
    }
#endif

    sp.value.reset();
}

void stuff_ctl::apply_axis_state(char key)
{
    auto &axis = axis_[key];

    if (!axis.in_use || !axis.value.has_value())
        return;

    if (node::is_transiting(axis.ctl))
        return;

    eng::log::info("{}: {} = {}", name(), key, *axis.value);

#ifdef BUILDROOT
    if (*axis.value)
    {
        if (node::is_ready(axis.ctl) || node::is_active(axis.ctl))
            node::activate(axis.ctl, { *axis.value });
    }
    else
    {
        if (node::is_active(axis.ctl))
            node::deactivate(axis.ctl);
    }
#endif

    axis.value.reset();
}

bool stuff_ctl::in_use_any() const
{
    if (fc_.in_use)
        return true;

    for (std::size_t i = 0; i < sp_.size(); ++i)
        if (sp_[i].in_use) return true;

    for (auto const &[ axis, desc ] : axis_)
        if (desc.in_use) return true;

    return false;
}

bool stuff_ctl::is_stuff_usable() const
{
    bool result = true;

    if (fc_.in_use && !node::is_ready(fc_.ctl))
    {
        eng::log::error("{}: ПЧ недоступен для работы", name());
        result = false;
    }

    for (std::size_t i = 0; i < sp_.size(); ++i)
    {
        if (sp_[i].in_use && !node::is_ready(sp_[i].ctl))
        {
            eng::log::error("{}: Спрейер №{} недоступен для работы", name(), i);
            result = false;
        }
    }

    for (auto const &[ axis, desc ] : axis_)
    {
        if (desc.in_use && !node::is_ready(desc.ctl))
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

void stuff_ctl::register_on_bus_done()
{
    node::ready(ictl_);
}

