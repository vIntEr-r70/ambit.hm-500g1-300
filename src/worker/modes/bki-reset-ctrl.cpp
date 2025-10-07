#include "bki-reset-ctrl.h"
#include "flags.h"

#include <hardware.h> 
#include <aem/log.h>

bki_reset_ctrl::bki_reset_ctrl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , hw_(hw)
    , reset_(hw_.get_unit_by_class(sid, "reset"), sid)
{ 
    hw.SET.add("bki", "reset", [this](nlohmann::json const &) {
        hw_.set_flag(flags::do_reset_bki, true);
    });
    
    hw.LSET.add(sid, "bki-lock", [this](nlohmann::json const &value) 
    {
        bki_lock_state_ = value.get<bool>();
         
        if (hw_.get_flag(flags::bki_not_allow) || hw_.get_flag(flags::bki_lock_ignore))
        {
            may_skip_bki_update_ = true;
            return;
        }
            
        hw_.rparam_touch("bki-lock", bki_lock_state_);
        hw_.notify({ "sys", "bki-lock" }, bki_lock_state_);
    });
} 

void bki_reset_ctrl::on_activate() noexcept
{
    bool turn_off_bki = hw_.get_flag(flags::bki_not_allow);
    next_state(turn_off_bki ? nullptr : &bki_reset_ctrl::clear_reset_bit);
}

void bki_reset_ctrl::on_deactivate() noexcept
{ }

void bki_reset_ctrl::wait_allow_reset() noexcept
{
    if (may_skip_bki_update_ && !hw_.get_flag(flags::bki_lock_ignore))
    {
        may_skip_bki_update_ = false;
        
        hw_.rparam_touch("bki-lock", bki_lock_state_);
        hw_.notify({ "sys", "bki-lock" }, bki_lock_state_);
    }
    
    if (!hw_.get_flag(flags::do_reset_bki))
        return;
    aem::log::trace("bki-reset-ctrl::set_reset_bit");
    next_state(&bki_reset_ctrl::set_reset_bit);
}

void bki_reset_ctrl::set_reset_bit() noexcept
{
    auto [ done, error ] = reset_.set(true);
    if (!dhresult::check(done, error, [this] { next_state(&bki_reset_ctrl::error); }))
        return;

    // Засекаем время
    countdown_ = aem::countdown(aem::time_span::ms(500));

    // Команда выполнена, идем на ожидание реакции 
    aem::log::trace("bki-reset-ctrl::wait_timeout");
    next_state(&bki_reset_ctrl::wait_timeout);
}

void bki_reset_ctrl::wait_timeout() noexcept
{
    if (!countdown_.elapsed())
        return;
    aem::log::trace("bki-reset-ctrl::clear_reset_bit");
    next_state(&bki_reset_ctrl::clear_reset_bit);
}

void bki_reset_ctrl::clear_reset_bit() noexcept
{
    auto [ done, error ] = reset_.set(false);
    if (!dhresult::check(done, error, [this] { next_state(&bki_reset_ctrl::error); }))
        return;
    hw_.set_flag(flags::do_reset_bki, false);
    aem::log::trace("bki-reset-ctrl::wait_allow_reset");
    next_state(&bki_reset_ctrl::wait_allow_reset);
}

void bki_reset_ctrl::error() noexcept
{
    hw_.log_message(LogMsg::Error, "Не удалось сбросить контроллер касания");
    hw_.set_flag(flags::do_reset_bki, false);
    next_state(&bki_reset_ctrl::wait_allow_reset);
}
