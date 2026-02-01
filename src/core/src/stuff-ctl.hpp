#pragma once

#include <eng/sibus/node.hpp>

#include <unordered_map>

class stuff_ctl final
    : public eng::sibus::node
{
    typedef void (stuff_ctl::*activate_command)(eng::abc::pack);

    eng::sibus::input_wire_id_t ictl_;

    struct unit_t
    {
        bool in_use;
        bool active;
        eng::sibus::output_wire_id_t ctl;
    };

    unit_t fc_;
    std::array<unit_t, 3> sp_;
    std::unordered_map<char, unit_t> axis_;

    void (stuff_ctl::*state_)();

public:

    stuff_ctl();

private:

    void activate(eng::abc::pack);

    void deactivate();

private:

    void do_command(eng::abc::pack);

    void cmd_operation(eng::abc::pack);

private:

    void s_ready_to_work();

    void s_in_proccess();

    void s_deactivating();

private:

    bool is_deactivated(unit_t &);

    bool is_state_correct(unit_t &);

private:

    void apply_fc_state(double, double);

    void apply_sp_state(std::size_t, bool);

    void apply_axis_state(char, double);

private:

    void load_axis_list();

    bool is_stuff_usable() const;
};
