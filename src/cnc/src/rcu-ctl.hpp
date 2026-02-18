#pragma once

#include <eng/sibus/node.hpp>
#include <eng/timer.hpp>

#include <bitset>
#include <optional>
#include <vector>

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

    eng::sibus::output_port_id_t led_;

    std::vector<double> filter_;
    eng::timer::id_t tid_;
    double last_speed_{ 0.0 };

public:

    rcu_ctl();

private:

    void update_axis_selection();

    char get_selected_tsp() const noexcept;

    char get_selected_axis(char) const noexcept;

private:

    void spin_value_changed(eng::abc::pack);

    void update_axis_position(char, double);

    void add_next_value(char, double);

    void reset_filter(char);

    void calculate_next_value(char);
};

