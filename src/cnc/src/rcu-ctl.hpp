#pragma once

#include <eng/sibus/node.hpp>

#include <bitset>
#include <optional>

class rcu_ctl final
    : public eng::sibus::node
{
    char axis_{ '\0' };

    struct axis_t
    {
        eng::sibus::output_wire_id_t ctl;
        char linked_tsp_key;
        std::array<double, 3> speed;
        double ratio;
    };
    std::unordered_map<char, axis_t> axis_map_;

    std::unordered_map<char, bool> tsp_select_map_;
    std::bitset<2> speed_select_;
    std::optional<std::int32_t> position_;

    eng::sibus::output_port_id_t led_out_;

public:

    rcu_ctl();

private:

    void update_axis_selection();

    char get_selected_tsp() const noexcept;

    char get_selected_axis(char) const noexcept;

private:

    void spin_value_changed(eng::abc::pack);
};

