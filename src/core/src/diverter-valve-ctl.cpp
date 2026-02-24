#include "diverter-valve-ctl.hpp"

#include <eng/log.hpp>

// Данный узел управляет положением клапана и насосом / насосами
// Имеем в управлении:
// - клапан 1 штука
// - насос 1 штука или 2 штуки
// - обратную связь по реальному положению клапана

diverter_valve_ctl::diverter_valve_ctl(std::string_view name, std::size_t H_count)
    : eng::sibus::node(name)
{
    // Требуемое положение клапана
    node::add_input_port("in", [this](eng::abc::pack args)
    {
        bool state = eng::abc::get<bool>(args);

        eng::log::info("{}: in = {}", node::name(), state);

        for (std::size_t i = 0; i < items_.size(); ++i)
            node::set_port_value(items_[i].VP, { state });
    });

    // Требуемое состояние насоса
    node::add_input_port("H", [this](eng::abc::pack args)
    {
        turn_on_off_pump(eng::abc::get<bool>(args));
    });

    // Управление насосами
    for (std::size_t i = 0; i < H_count; ++i)
    {
        // Реальное положение задвижки
        node::add_input_port(std::format("VP{}", i + 1), [this, i](eng::abc::pack args)
        {
            // 0, 1, 2, 3 - возможные значения положения задвижки
            auto state = eng::abc::get<std::uint8_t>(args);
            eng::log::info("{}: VP{} = {}", node::name(), i, state);
            items_[i].vp_real = (state == 1 || state == 2) ? state : 0;
            turn_on_off_pump(false);
        });

        auto h_id = node::add_output_port(std::format("H{}", i + 1));
        auto vp_id = node::add_output_port(std::format("VP{}", i + 1));

        items_.emplace_back(h_id, vp_id, 0);
    }
}

void diverter_valve_ctl::turn_on_off_pump(bool value)
{
    eng::log::info("{}: {}", name(), __func__);

    // Если насос один
    if (items_.size() == 1)
    {
        bool pump_state = (items_[0].vp_real != 0) && value;
        node::set_port_value(items_[0].H, { pump_state });
    }
    // Если насосов два то положение задвижки определяет какой нам надо включить
    else
    {
        std::size_t idx = (items_[0].vp_real == items_[1].vp_real) ? items_[0].vp_real : 0;
        for (std::size_t i = 0; i < items_.size(); ++i)
        {
            bool pump_state = value && (idx == (i + 1));
            node::set_port_value(items_[i].H, { pump_state });
        }
    }
}
