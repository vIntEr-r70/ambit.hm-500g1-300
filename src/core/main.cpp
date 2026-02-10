#include "src/stuff-ctl.hpp"
#include "src/intervals.hpp"
#include "src/auto-mode/auto-mode.hpp"
#include "src/speed-node.hpp"
#include "src/panel-button-ctl.hpp"
#include "src/panel-switch-2-ctl.hpp"
#include "src/current-conductors.hpp"
#include "src/working-mode-selector.hpp"

#include <eng/sibus/client.hpp>
#include <eng/eng.hpp>
#include <eng/log.hpp>

auto main() -> int
{
    char const *LIAEM_RO_PATH = std::getenv("LIAEM_RO_PATH");
    if (!LIAEM_RO_PATH)
    {
        eng::log::error("no LIAEM_RO_PATH enviroment variable");
        return 1;
    }
    std::filesystem::path conf_path(LIAEM_RO_PATH);

    eng::sibus::client::init();

    auto_mode auto_mode;
    stuff_ctl stuff_ctl;

    panel_button_ctl sp0("btn-sp0-ctl");
    panel_button_ctl sp1("btn-sp1-ctl");
    panel_button_ctl sp2("btn-sp2-ctl");

    panel_button_ctl h1("btn-H1-ctl");
    panel_button_ctl h2("btn-H3-ctl");

    panel_switch_2_ctl ps0{ "coolant-selector" };
    panel_switch_2_ctl ps1{ "drain-selector" };

    current_conductors cc0;

    working_mode_selector wms0;

    intervals intervals(conf_path);

    speed_node speed_x{ 'X' };
    speed_node speed_y{ 'Y' };
    speed_node speed_z{ 'Z' };
    speed_node speed_v{ 'V' };

    return eng::run();
}

