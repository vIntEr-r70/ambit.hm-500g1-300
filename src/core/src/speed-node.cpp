#include "speed-node.hpp"

#include <eng/log.hpp>
#include <eng/sibus/client.hpp>
#include <ambit/common/axis-config.hpp>

speed_node::speed_node(char axis)
    : eng::sibus::node(std::format("SPEED-{}", axis))
{
    std::string key = std::format("axis/{}", axis);
    eng::sibus::client::config_listener(key, [this](std::string_view json)
    {
        if (json.empty()) return;

        axis_config::desc desc;
        axis_config::load(desc, json);

        for (std::size_t idx = 0; idx < speed_.size(); ++idx)
            speed_[idx] = desc.speed[idx];

        update_output_value();
    });

    add_input_port("0", [this](eng::abc::pack value)
    {
        input_.ss0 = eng::abc::get<bool>(value);
        update_output_value();
    });

    add_input_port("1", [this](eng::abc::pack value)
    {
        input_.ss1 = eng::abc::get<bool>(value);
        update_output_value();
    });

    port_value_ = add_output_port("value");
}

void speed_node::update_output_value()
{
    std::println("------->speed_node::update_output_value: {}", speed_[input_.speed_idx()]);
    node::set_port_value(port_value_, { speed_[input_.speed_idx()] });
}

