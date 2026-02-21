#include "ethercat.hpp"
#include "functions/axis-motion-node.hpp"
#include "slaves/servo-motors/lc10e.hpp"

#include <eng/eng.hpp>
#include <eng/sibus/client.hpp>

auto main(int argc, char *argv[]) -> int
{
    eng::sibus::client::init();

    lc10e ctlX({ .alias = 0, .position = 0 });
    axis_motion_node motion_X('X', ctlX);

    lc10e ctlY({ .alias = 0, .position = 1 });
    axis_motion_node motion_Y('Y', ctlY);

    lc10e ctlZ({ .alias = 0, .position = 2 });
    axis_motion_node motion_Z('Z', ctlZ);

    lc10e ctlV({ .alias = 0, .position = 3 });
    axis_motion_node motion_V('V', ctlV);

    ethercat::init();

    return eng::run();
}

