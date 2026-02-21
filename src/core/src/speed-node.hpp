#pragma once

#include <eng/sibus/node.hpp>

class speed_node final
    : public eng::sibus::node
{
    eng::sibus::output_port_id_t value_;

    // Состояние входов
    struct input_t
    {
        bool ss0{ false };
        bool ss1{ false };

        std::size_t speed_idx() const noexcept {
            return (ss0 != ss1) ? (ss0 ? 1 : 2) : 0;
        }
    };
    input_t input_;

    std::array<double, 3> panel_speed_{ 0.0, 0.0, 0.0 };
    std::array<double, 3> rcu_speed_{ 0.0, 0.0, 0.0 };
    std::array<double, 3> *speed_{ nullptr };

    std::string auto_mode_;
    std::string rcu_mode_;

public:

    speed_node(char);

private:

    void update_speed_select();

    void update_output_value();
};

