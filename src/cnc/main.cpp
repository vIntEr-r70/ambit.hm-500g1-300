#include "src/axis-panel-ctl.hpp"
#include "src/rcu-ctl.hpp"
#include "src/multi-axis-ctl.hpp"

#include <eng/sibus/client.hpp>
#include <eng/eng.hpp>

auto main() -> int
{
    eng::sibus::client::init();

    axis_panel_ctl axis_panel_ctl_x('X');
    axis_panel_ctl axis_panel_ctl_y('Y');
    axis_panel_ctl axis_panel_ctl_z('Z');
    axis_panel_ctl axis_panel_ctl_v('V');

    multi_axis_ctl multi_axis_ctl;

    rcu_ctl rcu_ctl;

    return eng::run();
}

