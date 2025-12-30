#pragma once

#include <InteractWidgets/InteractWidget.h>

#include <eng/sibus/node.hpp>

class QLabel;
class QLineEdit;
class RoundButton;
class QStackedWidget;
class NumberCalcField;
class axis_ctl_widget;

class manual_engine_set_dlg
    : public InteractWidget 
    , public eng::sibus::node
{
    Q_OBJECT

    axis_ctl_widget *axis_ctl_;

    eng::sibus::output_wire_id_t owire_;

public:

    manual_engine_set_dlg(QWidget *);

public:

    void set_axis(char, std::string_view);

    void set_axis_speed(char, double);

    void select_axis(char);

signals:

    void axis_ctl_access(bool);

private:

    void register_on_bus_done() override final;
};


