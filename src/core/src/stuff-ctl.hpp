#pragma once

#include <eng/sibus/node.hpp>

#include <string_view>
#include <unordered_map>

class stuff_ctl final
    : public eng::sibus::node
{
    eng::sibus::input_wire_id_t ictl_;

    struct unit_t
    {
        bool in_use;
        bool in_active;
        eng::sibus::output_wire_id_t ctl;
        std::string emsg;
    };

    unit_t fc_;
    std::array<unit_t, 3> sp_;
    std::unordered_map<char, unit_t> axis_;

    std::unordered_map<char, std::string> axis_name_;

    bool initialized_{ false };

public:

    stuff_ctl();

private:

    void activate(eng::abc::pack);

    void deactivate_all();

    void terminate_execution(std::string_view, std::string_view);

private:

    void apply_state(eng::abc::pack);

    void apply_fc_state(double, double);

    void apply_sp_state(std::size_t, bool);

    void apply_axis_state(char, double);

private:

    void ctl_fc_state(std::string_view);

    void ctl_sp_state(std::size_t, std::string_view);

    void ctl_axis_state(char, std::string_view);

private:

    bool is_stuff_usable();

    bool in_use_any() const;

private:

    void register_on_bus_done() override final;
};
