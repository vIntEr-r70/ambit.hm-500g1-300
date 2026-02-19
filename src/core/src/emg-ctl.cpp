#include "emg-ctl.hpp"

emg_ctl::emg_ctl()
    : eng::sibus::node("emg-ctl")
{
    emg_ = node::add_output_port("emg");

    node::add_input_port("emg", [this](eng::abc::pack args)
    {
        bool active = !eng::abc::get<bool>(args);
        node::set_port_value(emg_, { active });
    });
}

