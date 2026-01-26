#pragma once

#include <InteractWidgets/InteractWidget.h>

#include <eng/abc/pack.hpp>

class QLabel;
class QLineEdit;
class RoundButton;
class QStackedWidget;
class NumberCalcField;
class axis_ctl_widget;

class manual_engine_set_dlg
    : public InteractWidget 
{
    Q_OBJECT

    axis_ctl_widget *axis_ctl_;

public:

    manual_engine_set_dlg(QWidget *);

public:

    void add_axis(char, std::string_view, bool);

    void set_axis_max_speed(char, double);

    void set_axis_position(char, double);

    void set_axis_real_speed(char, double);

    void select_axis(char);

signals:

    void axis_command(char, eng::abc::pack);

// private:
//
//     void register_on_bus_done() override final;
};


