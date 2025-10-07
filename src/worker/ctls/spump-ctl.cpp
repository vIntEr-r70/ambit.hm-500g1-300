#include "spump-ctl.h"

#include <hardware.h>
#include <aem/log.h>

spump_ctl::spump_ctl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , sid_(sid)
    , hw_(hw)
    , env_(hw.get_unit_by_class(sid, "env-pump"), "env-pump")
    , wtr_(hw.get_unit_by_class(sid, "wtr-pump"), "wtr-pump")
    , led_(hw.get_unit_by_class(sid, "spump-led"), "spump-led")
{ 
    hw.START.add(sid_, [this] 
    { 
        hw_.log_message(LogMsg::Command, sid_, "Включение");
        cmd_ = true; 
    });

    hw.STOP.add(sid_, [this] 
    { 
        hw_.log_message(LogMsg::Command, sid_, "Выключение");
        cmd_ = false; 
    });

    hw.LSET.add(sid_, "start", [this](nlohmann::json const &value)
    { 
        if (!value.get<bool>()) return;
        hw_.log_message(LogMsg::Command, sid_, "Включение");
        cmd_ = true; 
    });
    
    hw.LSET.add(sid_, "stop", [this](nlohmann::json const &value) 
    { 
        if (!value.get<bool>()) return;
        hw_.log_message(LogMsg::Command, sid_, "Выключение");
        cmd_ = false; 
    });
    
    // Откуда сливать
    hw.LSET.add(sid, "tank", [this](nlohmann::json const &value) 
    {
        hw_.log_message(LogMsg::Info, sid_, fmt::format("tank: {}", value.dump()));
        tank_ = value.get<bool>();
    });
    
    hw.listen_event(sid, "start", [this](nlohmann::json const &value) 
    {
        bool start = value.get<bool>();
        hw_.log_message(LogMsg::Command, sid_, start ? "Включение" : "Выключение");
    });
}

void spump_ctl::on_activate() noexcept
{
    env_.reset();
    wtr_.reset();
    led_.reset();

    init_tasks_.push_back(&env_);
    init_tasks_.push_back(&wtr_);
    init_tasks_.push_back(&led_);
    
    next_state(&spump_ctl::turn_all_off);
}

void spump_ctl::on_deactivate() noexcept
{ }

void spump_ctl::turn_all_off() noexcept
{
    if (init_tasks_.empty())
    {
        next_state(&spump_ctl::turned_off);
        return;
    }
    
    auto [ done, error ] = init_tasks_.back()->set(false);
    if (!dhresult::check(done, error, [this] { set_error("Не удалось выключить"); }))
        return;

    init_tasks_.pop_back();
}

void spump_ctl::turn_on_env_pump() noexcept
{
    auto [ done, error ] = env_.set(true);
    if (!dhresult::check(done, error, [this] { set_error("Не удалось включить насос среды"); }))
        return;
    next_state(&spump_ctl::turn_on_led);
}

void spump_ctl::turn_on_wtr_pump() noexcept
{
    auto [ done, error ] = wtr_.set(true);
    if (!dhresult::check(done, error, [this] { set_error("Не удалось включить насос воды"); }))
        return;
    next_state(&spump_ctl::turn_on_led);
}

void spump_ctl::turn_on_led() noexcept
{
    auto [ done, error ] = led_.set(true);
    if (!dhresult::check(done, error, [this] { next_state(&spump_ctl::turned_on); }))
        return;
    next_state(&spump_ctl::turned_on);
}

void spump_ctl::turned_on() noexcept
{
    // Ждем команды
    if (!cmd_.has_value()) 
        return;
    
    bool turn_on = cmd_.value();
    cmd_.reset();
    
    // Пришла команда на включение 
    if (turn_on == true)
        return;

    init_tasks_.push_back(&env_);
    init_tasks_.push_back(&wtr_);
    init_tasks_.push_back(&led_);
    
    next_state(&spump_ctl::turn_all_off);
}

void spump_ctl::turned_off() noexcept
{
    // Ждем команды
    if (!cmd_.has_value()) 
        return;
    
    bool turn_on = cmd_.value();
    cmd_.reset();
    
    // Пришла команда на выключение 
    if (turn_on == false)
        return;

    if (tank_)
        next_state(&spump_ctl::turn_on_env_pump);
    else
        next_state(&spump_ctl::turn_on_wtr_pump);
}

void spump_ctl::set_error(std::string_view emsg) noexcept
{
    hw_.log_message(LogMsg::Error, sid_, emsg);
    next_state(nullptr);
}

