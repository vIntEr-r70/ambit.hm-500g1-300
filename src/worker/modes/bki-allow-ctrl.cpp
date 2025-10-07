#include "bki-allow-ctrl.h"
#include "flags.h"

#include <hardware.h> 
#include <aem/log.h>

bki_allow_ctrl::bki_allow_ctrl(std::string_view sid, we::hardware &hw) noexcept
    : state_ctrl(sid)
    , hw_(hw)
    , allow_(hw_.get_unit_by_class(sid, "allow"), sid)
{ } 

void bki_allow_ctrl::on_activate() noexcept
{
    next_state(&bki_allow_ctrl::bki_allow);
}

void bki_allow_ctrl::on_deactivate() noexcept
{ }

void bki_allow_ctrl::bki_allow() noexcept
{
    auto [ done, error ] = allow_.set(!hw_.get_flag(flags::bki_not_allow));
    if (!dhresult::check(done, error, [this] { next_state(&bki_allow_ctrl::error); }))
        return;
    next_state(nullptr);
}

void bki_allow_ctrl::error() noexcept
{
    hw_.log_message(LogMsg::Error, "bki", "Не удалось инициализировать контроллер БКИ");
    next_state(nullptr);
}
