#pragma once

#include <eng/sibus/node.hpp>

class multi_mode_panel_btns final
    : public eng::sibus::node
{
    typedef void (multi_mode_panel_btns::*handler_t)();

    struct handlers_t
    {
        handler_t start;
        handler_t stop;

        eng::sibus::output_wire_id_t ctl;
    };

    handlers_t fc_{
        &multi_mode_panel_btns::fc_turn_on_action,
        &multi_mode_panel_btns::fc_turn_off_action
    };

    handlers_t auto_{
        &multi_mode_panel_btns::auto_turn_on_action,
        &multi_mode_panel_btns::auto_turn_off_action
    };

    handlers_t *handlers_{ nullptr };

public:

    multi_mode_panel_btns();

private:

    void fc_turn_on_action();

    void fc_turn_off_action();

    void auto_turn_on_action();

    void auto_turn_off_action();
};
