#include "current-conductors.hpp"

#include <eng/sibus/client.hpp>
#include <eng/json.hpp>
#include <eng/log.hpp>

current_conductors::current_conductors()
    : eng::sibus::node("current-conductors")
{
    std::string key{ "fc/current-conductors" };
    eng::sibus::client::config_listener(key, [this](std::string_view json)
    {
        eng::json::array cfg(json);
        cfg.for_each([this](std::size_t, eng::json::value v)
        {
            eng::json::object item(v);

            auto key = item.get_sv("key", "");
            double I = item.get<double>("I", 0.0);
            double U = item.get<double>("U", 0.0);

            records_.emplace_back(std::string(key), I, U);
        });

        update_output();
    });

    for (std::size_t i = 0; i < selected_.size(); ++i)
    {
        node::add_input_port(std::to_string(i), [this, i](eng::abc::pack args)
        {
            selected_.set(i, eng::abc::get<bool>(args));
            update_output();
        });
    }

    p_out_ = node::add_output_port("limits");
}

void current_conductors::update_output()
{
    if (records_.empty())
        return;

    std::size_t idx = selected_.to_ulong();

    if (idx >= records_.size())
        idx = records_.size() - 1;

    auto const &r = records_[idx];
    node::set_port_value(p_out_, { r.key, r.I, r.U });

    eng::log::info("{}: key = {}, I = {}, U = {}",
            name(), r.key, r.I, r.U);
}

