#pragma once

#include <eng/sibus/node.hpp>

#include <string_view>
#include <unordered_map>

class stuff_ctl final
    : public eng::sibus::node
{
    using commands_handler = void(stuff_ctl::*)(eng::abc::pack const &);

    eng::sibus::input_wire_id_t ictl_;

    struct unit_t
    {
        bool in_use{ false };
        bool in_active{ false };
        eng::sibus::output_wire_id_t ctl;
        std::string emsg;
    };

    unit_t fc_;
    std::array<unit_t, 3> sp_;
    std::unordered_map<char, unit_t> axis_;

    std::unordered_map<char, std::string> axis_name_;

    bool axis_load_ok_;

public:

    stuff_ctl();

private:

    void start_command_execution(eng::abc::pack);

    void deactivate_all();

    void terminate_execution(std::string_view, std::string_view);

private:

    void cmd_prepare(eng::abc::pack const &);

    void cmd_operation(eng::abc::pack const &);

private:

    void apply_fc_state(double, double);

    void apply_sp_state(std::size_t, bool);

    void apply_axis_state(char, double);

private:

    void ctl_fc_state(std::string_view);

    void ctl_sp_state(std::size_t, std::string_view);

    void ctl_axis_state(char, std::string_view);

private:

    bool in_use_any() const;

private:

    void register_on_bus_done() override final;
};
