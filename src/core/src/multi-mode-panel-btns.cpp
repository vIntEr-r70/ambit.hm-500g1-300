#include "multi-mode-panel-btns.hpp"

#include <eng/sibus/client.hpp>
#include <eng/json.hpp>

// Кнопки СТАРТ/СТОП, которые в зависимости от выбранного режима
// включают и выключают ПЧ или Запускают и останавливают автоматический режим
// С ПЧ работают в режиме ручного управления
// С Авто работают когда выбрана программа в Gui

multi_mode_panel_btns::multi_mode_panel_btns()
    : eng::sibus::node("multi-mode-panel-btns")
{

    // Вход команды включения
    node::add_input_port("0", [this](eng::abc::pack args)
    {
        if (eng::abc::get<bool>(args) == true && handlers_)
            (this->*handlers_->start)();
    });

    // Вход команды выключения
    node::add_input_port("1", [this](eng::abc::pack args)
    {
        if (eng::abc::get<bool>(args) == false && handlers_)
            (this->*handlers_->stop)();
    });

    fc_.ctl = node::add_output_wire("fc");
    auto_.ctl = node::add_output_wire("auto");

    eng::sibus::client::config_listener("gui/fc/setpoints", [this](std::string_view json)
    {
        eng::json::object cfg(json);
        auto current = cfg.get<double>("current", NAN);
        auto power = cfg.get<double>("power", NAN);
        fc_sets_ = { current, power };
        if (node::is_active())
    });
}

void multi_mode_panel_btns::fc_turn_on_action()
{
    // Игнорируем команду если уставки не были заданы
    if (!fc_sets_) return;

    double i = fc_sets_.value().i;
    double p = fc_sets_.value().p;

    node::activate(fc_.ctl, { i, p });
}

void multi_mode_panel_btns::fc_turn_off_action()
{
    node::deactivate(fc_.ctl);
}

void multi_mode_panel_btns::auto_turn_on_action()
{
    node::activate(auto_.ctl, {});
}

void multi_mode_panel_btns::auto_turn_off_action()
{
    node::deactivate(auto_.ctl);
}
