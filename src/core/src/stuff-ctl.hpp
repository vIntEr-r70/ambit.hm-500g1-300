#pragma once

#include "common/internal-state-ctl.hpp"

#include <eng/sibus/node.hpp>

#include <unordered_map>

class stuff_ctl final
    : public eng::sibus::node
    , public internal_state_ctl<stuff_ctl>
{
    typedef void (stuff_ctl::*activate_command)(eng::abc::pack);

    eng::sibus::input_wire_id_t ictl_;

    template <typename T>
    struct unit_t
    {
        bool in_use;
        std::optional<T> value;
        eng::sibus::output_wire_id_t ctl;
    };

    struct fc_set_t
    {
        double i;
        double p;
    };
    unit_t<fc_set_t> fc_;

    std::array<unit_t<bool>, 3> sp_;
    std::unordered_map<char, unit_t<double>> axis_;

public:

    stuff_ctl();

private:

    void activate(eng::abc::pack);

    void deactivate();

private:

    void do_command(eng::abc::pack);

    void cmd_operation(eng::abc::pack);

private:

    void s_wait_system_ready();

    void s_lazy_state();

    void s_ctl_state();

private:

    void apply_fc_state();

    void apply_sp_state(std::size_t);

    void apply_axis_state(char);

private:

    void load_axis_list();

    bool is_stuff_usable() const;

    bool in_use_any() const;

private:

    void register_on_bus_done() override final;
};
