#pragma once

#include <eng/sibus/node.hpp>

class speed_node final
    : public eng::sibus::node
{
    eng::sibus::output_port_id_t port_value_;

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

    std::array<double, 3> speed_{ 0.0, 0.0, 0.0 };

public:

    speed_node(char);

private:

    void update_output_value();
};

