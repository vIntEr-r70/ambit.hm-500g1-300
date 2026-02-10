#include "working-mode-selector.hpp"

working_mode_selector::working_mode_selector()
    : eng::sibus::node("working-mode-selector")
{
    for (std::size_t i = 0; i < selected_.size(); ++i)
    {
        node::add_input_port(std::to_string(i), [this, i](eng::abc::pack args)
        {
            selected_.set(i, eng::abc::get<bool>(args));
            update_output();
        });
    }

    //////////////////////
    // | 0 | 1 |         |
    // |-------|---------|
    // | 0 | 0 | manual  |
    // | 1 | 0 | auto    |
    // | 0 | 1 | rcu     |
    // | 1 | 1 | manual  |
    //////////////////////
    out_[0] = node::add_output_port("manual");
    out_[1] = node::add_output_port("auto");
    out_[2] = node::add_output_port("rcu");
    out_[3] = out_[0];

    mode_out_ = node::add_output_port("mode");
}

void working_mode_selector::update_output()
{
    if (current_selected_ && current_selected_ == selected_.to_ulong())
        return;

    if (current_selected_)
        node::set_port_value(out_[current_selected_.value()], { false });

    current_selected_ = selected_.to_ulong();
    node::set_port_value(out_[current_selected_.value()], { true });

    auto const &mode = node::port_name(out_[current_selected_.value()]);
    node::set_port_value(mode_out_, { mode });
}
