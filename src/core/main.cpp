#include "src/stuff-ctl.hpp"
#include "src/sprayer-ctl.hpp"
#include "src/intervals.hpp"
#include "src/auto-mode/auto-mode.hpp"
#include "src/speed-node.hpp"
#include "src/panel-button-ctl.hpp"
#include "src/panel-switch-2-ctl.hpp"
#include "src/current-conductors.hpp"
#include "src/working-mode-selector.hpp"
#include "src/barrel-ctl.hpp"
#include "src/barrel-lvl-ctl.hpp"
#include "src/drainage-ctl.hpp"
#include "src/error-mask.hpp"
#include "src/bki-ctl.hpp"
#include "src/emg-ctl.hpp"
#include "src/diverter-valve-ctl.hpp"

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

    drainage_ctl dc0;

    barrel_ctl bc0{ "fc" };
    barrel_ctl bc1{ "water" };
    barrel_ctl bc2{ "fluid" };

    barrel_lvl_ctl blc0{ "fc" };
    barrel_lvl_ctl blc1{ "water" };
    barrel_lvl_ctl blc2{ "fluid" };

    sprayer_ctl spc0("sp0-ctl");
    sprayer_ctl spc1("sp1-ctl");
    sprayer_ctl spc2("sp2-ctl");

    error_mask em0;

    bki_ctl bki;
    emg_ctl emg;

    diverter_valve_ctl dvc0{ "valve-ctl-drainage", 1 };
    diverter_valve_ctl dvc1{ "valve-ctl-sprayers", 2 };

    return eng::run();
}

