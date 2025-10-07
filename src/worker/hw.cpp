#include "hw.h"

#include <aem/environment.h>
#include <we/controller.h>
#include <sc/fc/fc.h>

#include "modes/unit-on-off-ctl.h"
#include "modes/sprayer-on-off-ctl.h"
#include "modes/dmg-led-ctl.h"
#include "modes/fc-led-ctl.h"

#include "ctls/drain-ctl.h"
#include "ctls/spump-ctl.h"
#include "ctls/barrel-ctl.h"
#include "ctls/pr-barrel-ctl.h"
#include "ctls/drainage-ctl.h"

hw *hw::instance_ = nullptr;

void hw::create() noexcept 
{
    instance_ = new hw();
    instance_->configure_self();
}

void hw::destroy() noexcept
{
    delete instance_;
    instance_ = nullptr;
}

hw::hw() noexcept
    : hardware()
{ }

we::controller_creator hw::get_controller_creator(std::string_view key)
{
    static std::unordered_map<std::string_view, we::controller_creator> const map {
        { "Sp",             &we::ctemplate<sprayer_on_off_ctl>::create },
        { "pump",           &we::ctemplate<unit_on_off_ctl>::create },
        { "spump",          &we::ctemplate<unit_on_off_ctl>::create },
        { "dmg-led-ctl",    &we::ctemplate<dmg_led_ctl>::create },
        { "fc-led-ctl",     &we::ctemplate<fc_led_ctl>::create },
        { "fc",             &we::ctemplate<sc::fc>::create },
        { "drain",          &we::ctemplate<drain_ctl>::create },
        { "spump-ctl",      &we::ctemplate<spump_ctl>::create },
        { "barrel-0",       &we::ctemplate<pr_barrel_ctl>::create },
        { "barrel-1",       &we::ctemplate<barrel_ctl>::create },
        { "barrel-2",       &we::ctemplate<barrel_ctl>::create },
        { "drainage",       &we::ctemplate<drainage_ctl>::create },
    };
    auto it = map.find(key);
    return (it != map.end()) ? it->second : hardware::get_controller_creator(key);
}

