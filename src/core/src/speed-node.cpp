#include "speed-node.hpp"
#include "common/axis-config.hpp"

#include <eng/log.hpp>
#include <eng/sibus/client.hpp>

speed_node::speed_node(char axis)
    : eng::sibus::node(std::format("SPEED-{}", axis))
{
    std::string key = std::format("axis/{}", axis);
    eng::sibus::client::config_listener(key, [this](std::string_view json)
    {
        if (json.empty()) return;

        axis_config::desc desc;
        axis_config::load(desc, json);

        for (std::size_t idx = 0; idx < panel_speed_.size(); ++idx)
            panel_speed_[idx] = desc.speed[idx];

        for (std::size_t idx = 0; idx < rcu_speed_.size(); ++idx)
            rcu_speed_[idx] = desc.rcu.speed[idx];

        update_output_value();
    });

    node::add_input_port("0", [this](eng::abc::pack value)
    {
        input_.ss0 = eng::abc::get<bool>(value);
        update_output_value();
    });

    node::add_input_port("1", [this](eng::abc::pack value)
    {
        input_.ss1 = eng::abc::get<bool>(value);
        update_output_value();
    });

    node::add_input_port_unsafe("rcu-mode", [this](eng::abc::pack args)
    {
        rcu_mode_ = args ? eng::abc::get_sv(args) : "";
        update_speed_select();
        update_output_value();
    });

    node::add_input_port_unsafe("auto-mode", [this](eng::abc::pack args)
    {
        auto_mode_ = args ? eng::abc::get_sv(args) : "";
        update_speed_select();
        update_output_value();
    });

    value_ = add_output_port("value");
}

void speed_node::update_speed_select()
{
    speed_ = nullptr;

    if (auto_mode_ == "auto")
        return;

    if (auto_mode_ == "manual")
    {
        if (rcu_mode_ == "rcu")
            speed_ = &rcu_speed_;
        else if (rcu_mode_ == "panel")
            speed_ = &panel_speed_;
    }
}

void speed_node::update_output_value()
{
    if (speed_ == nullptr)
        node::set_port_value(value_, { });
    else
        node::set_port_value(value_, { (*speed_)[input_.speed_idx()] });
}

