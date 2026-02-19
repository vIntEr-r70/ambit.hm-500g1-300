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
        pump_ = eng::abc::get<bool>(args);
        // update_state();
    });

    // Управление насосами
    for (std::size_t i = 0; i < H_count; ++i)
    {
        // Реальное положение задвижки
        node::add_input_port(std::format("VP{}", i + 1), [this, i](eng::abc::pack args)
        {
            auto state = eng::abc::get<std::uint8_t>(args);
            // vp_real_ = eng::abc::get<std::uint8_t>(args);
            eng::log::info("{}: VP{} = {}", node::name(), i, state);
            // update_state();
        });

        auto h_id = node::add_output_port(std::format("H{}", i + 1));
        auto vp_id = node::add_output_port(std::format("VP{}", i + 1));

        items_.emplace_back(h_id, vp_id);
    }
}

void diverter_valve_ctl::update_state()
{
    // eng::log::info("{}: {}", name(), __func__);
    //
    // // Если мы не знаем реального положения задвижки,
    // // ничего делать мы не имеем права
    // if (!vp_real_ || H_.empty())
    //     return;
    //
    // // Задвижка находится в процессе переключения
    // if (*vp_real_ != vp_.to_ulong())
    //     return;
    //
    // // От нас хотят странного
    // if (vp_.count() != 1)
    //     return;
    //
    // // Если необходимо изменить состояние насоса
    // if (pump_)
    // {
    //     // Определяем какой насос должен быть активирован
    //     // исходя из положения задвижки
    //     std::size_t idx = vp_.test(0) ? 0 : 1;
    //     idx = std::min(idx, H_.size() - 1);
    //
    //     node::set_port_value(H_[idx], { pump_.value() });
    //
    //     pump_.reset();
    // }
}
