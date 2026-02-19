#pragma once

#include <eng/sibus/node.hpp>

#include <bitset>

class diverter_valve_ctl final
    : public eng::sibus::node
{
    // Требуемое положение задвижки
    std::bitset<2> vp_;

    std::optional<std::size_t> vp_real_;

    // Требуемое состояние насоса
    std::optional<bool> pump_;

    struct item_t
    {
        eng::sibus::output_port_id_t H;
        eng::sibus::output_port_id_t VP;
    };
    std::vector<item_t> items_;

public:

    diverter_valve_ctl(std::string_view, std::size_t);

private:

    void update_state();
};
