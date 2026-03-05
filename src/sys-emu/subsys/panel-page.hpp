#pragma once

#include <QWidget>

class pair_buttons;
class select_buttons;

class panel_page final
    : public QWidget
{
public:

    panel_page(QWidget *);

private:

    void init_fc_power(pair_buttons *);

    void init_pump_fc(pair_buttons *);

    void init_pump_sprayer(pair_buttons *);

    void init_emergency_stop(pair_buttons *);

    void init_multi_button(pair_buttons *);

    void init_sprayer_0(pair_buttons *);

    void init_sprayer_1(pair_buttons *);

    void init_sprayer_2(pair_buttons *);

    void init_speed_select(select_buttons *);

    void init_mode_select(select_buttons *);

    void init_rcu_select(select_buttons *);

    void init_environment_select(select_buttons *);

    void init_drainage_select(select_buttons *);
};
