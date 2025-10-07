#include "pr-barrel-ctl.h"

#include <cmath>
#include <hardware.h>
#include <aem/log.h>

pr_barrel_ctl::pr_barrel_ctl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , hw_(hw)
    , add_liquid_ctl_(hw.get_unit_by_class(sid, "add-liquid"), "add-liquid")
    , temp_min_ctl_(hw_.get_unit_by_class(sid, "temp-min"), "temp-min")
    , temp_max_ctl_(hw_.get_unit_by_class(sid, "temp-max"), "temp-max")
    , activate_ctl_(hw_.get_unit_by_class(sid, "activate"), "activate")
{ 
    hw.LSET.add(sid, "dt", [this](nlohmann::json const &value) {
        hw_.notify({ name(), "dt" }, value);
    });
    // Читаем текущее общее долитое количество жидкости
    hw.LSET.add(sid, "liquid", [this](nlohmann::json const &value) {
        hw_.notify({ name(), "liquid" }, value);
    });
    // Читаем текущее оставшееся долиться количество жидкости
    hw.LSET.add(sid, "add-liquid", [this](nlohmann::json const &value) {
        hw_.notify({ name(), "add-liquid" }, value);
    });
    
    hw.START.add(sid, [this] 
    { 
        op_activate_ = true;
    });

    hw.STOP.add(sid, [this] 
    { 
        op_activate_ = false;
    });
    
    // Команда долить жидкости
    hw.SET.add(sid, "add-liquid", [this](nlohmann::json const &json) 
    {
        std::uint16_t rounded = static_cast<std::uint16_t>(std::lround(json.get<double>()));
        tasks_.push_back({ &add_liquid_ctl_, rounded, fmt::format("Доливаем {} литров", rounded) });
    });

    // Команда долить жидкости
    hw.SET.add(sid, "use-heater", [this](nlohmann::json const &json) 
    {
        // hw_.save(name(), "use-heater", json);
        // use_heater_ = json.get<bool>();
        // hw_.notify({ name(), "use-heater" }, use_heater_);
    });
    hw.SET.add(sid, "use-cooler", [this](nlohmann::json const &json) 
    {
        // hw_.save(name(), "use-cooler", json);
        // use_cooler_ = json.get<bool>();
        // hw_.notify({ name(), "use-cooler" }, use_cooler_);
    });
    hw.SET.add(sid, "full-time-pump", [this](nlohmann::json const &json) 
    {
        // hw_.save(name(), "full-time-pump", json);
        // bool v = json.get<bool>();
        // hw_.notify({ name(), "full-time-pump" }, v);
    });
    hw.SET.add(sid, "dt-max", [this](nlohmann::json const &json) 
    {
        hw_.save(name(), "dt-max", json);
        temp_max_ = json.get<float>();
        hw_.notify({ name(), "dt-max" }, temp_max_);
        update_temp_max();
    });
    hw.SET.add(sid, "dt-min", [this](nlohmann::json const &json) 
    {
        hw_.save(name(), "dt-min", json);
        temp_min_ = json.get<float>();
        hw_.notify({ name(), "dt-min" }, temp_min_);
        update_temp_min();
    });

    // use_heater_ = hw_.storage(name(), "use-heater", false);
    // hw_.notify({ name(), "use-heater" }, use_heater_);
    
    // use_cooler_ = hw_.storage(name(), "use-cooler", false);
    // hw_.notify({ name(), "use-cooler" }, use_cooler_);

    // bool flag = hw_.storage(name(), "full-time-pump", false);
    // hw_.notify({ name(), "full-time-pump" }, flag);
    
    temp_min_ = hw_.storage(name(), "dt-min", 20.0f);
    hw_.notify({ name(), "dt-min" }, temp_min_);
    
    temp_max_ = hw_.storage(name(), "dt-max", 80.0f);
    hw_.notify({ name(), "dt-max" }, temp_max_);
}

void pr_barrel_ctl::on_activate() noexcept
{
    update_temp_min();
    update_temp_max();
        
    hw_.log_message(LogMsg::Info, name(), "Контроллер успешно активирован");
    next_state(&pr_barrel_ctl::wait_new_tasks);
}

void pr_barrel_ctl::on_deactivate() noexcept
{ 
    hw_.log_message(LogMsg::Info, name(), "Контроллер деактивирован");
}

void pr_barrel_ctl::wait_new_tasks()
{
    if (tasks_.empty())
    {
        if (op_activate_)
        {
            activate_ = op_activate_.value();
            op_activate_.reset();
            
            next_state(&pr_barrel_ctl::do_activate);
        }
        
        return;
    }
    task_id_ = 0;
    
    next_state(&pr_barrel_ctl::do_task);
}

void pr_barrel_ctl::do_task()
{
    if (task_id_ >= tasks_.size())
    {
        tasks_.clear();
        next_state(&pr_barrel_ctl::wait_new_tasks);
        return;
    }
    
    auto &task = tasks_[task_id_];
    
    auto [ done, error ] = task.driver->set(task.value);
    if (!dhresult::check(done, error, [this] { next_state(&pr_barrel_ctl::do_task_error); }))
        return;

    task_id_ += 1;
    hw_.log_message(LogMsg::Info, name(), task.message);
}

void pr_barrel_ctl::do_task_error()
{
    task_id_ += 1;
    hw_.log_message(LogMsg::Error, name(), "Не удалось выполнить задание");
    
    next_state(&pr_barrel_ctl::do_task);
}

void pr_barrel_ctl::do_activate()
{
    auto [ done, error ] = activate_ctl_.set(activate_);
    if (!dhresult::check(done, error, [this] { next_state(&pr_barrel_ctl::do_activate_error); }))
        return;
    hw_.log_message(LogMsg::Info, name(), fmt::format("Контроль {}", activate_ ? "активирован" : "отключен"));
    next_state(&pr_barrel_ctl::wait_new_tasks);
}

void pr_barrel_ctl::do_activate_error()
{
    hw_.log_message(LogMsg::Error, name(), fmt::format("Не удалось {} контроль", activate_ ? "активировать" : "отключить"));
    next_state(&pr_barrel_ctl::wait_new_tasks);
}

void pr_barrel_ctl::update_temp_min()
{
    if (temp_max_ <= temp_min_) return;
    std::uint16_t rounded = static_cast<std::uint16_t>(std::lround(temp_min_));
    tasks_.push_back({ &temp_min_ctl_, rounded, fmt::format("Задание нижней границы температуры: {}", rounded) });
}

void pr_barrel_ctl::update_temp_max()
{
    if (temp_max_ <= temp_min_) return;
    std::uint16_t rounded = static_cast<std::uint16_t>(std::lround(temp_max_));
    tasks_.push_back({ &temp_max_ctl_, rounded, fmt::format("Задание верхней границы температуры: {}", rounded) });
}


