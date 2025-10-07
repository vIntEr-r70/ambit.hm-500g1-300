#include "sprayer-on-off-ctl.h"

#include <hardware.h>

sprayer_on_off_ctl::sprayer_on_off_ctl(std::string_view sid, we::hardware &hw) noexcept
    : unit_on_off_ctl(sid, hw)
{ }

bool sprayer_on_off_ctl::is_locked() const noexcept
{
    // return hw_.is_locked(we::locker::lock_bits::sprayer);
    return false;
}
