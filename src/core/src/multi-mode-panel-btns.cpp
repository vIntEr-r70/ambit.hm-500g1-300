#include "multi-mode-panel-btns.hpp"

// Кнопки СТАРТ/СТОП, которые в зависимости от выбранного режима
// включают и выключают ПЧ или Запускают и останавливают автоматический режим
// С ПЧ работают в режиме ручного управления
// С Авто работают когда выбрана программа в Gui

multi_mode_panel_btns::multi_mode_panel_btns()
    : eng::sibus::node("multi-mode-panel-btns")
{
    node::add_input_port("auto", [this](eng::abc::pack args)
    {
        if (eng::abc::get<bool>(args))
            handlers_ = &auto_;
        else
            handlers_ = &fc_;
    });

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
}

void multi_mode_panel_btns::fc_turn_on_action()
{
    node::activate(fc_.ctl, { });
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
