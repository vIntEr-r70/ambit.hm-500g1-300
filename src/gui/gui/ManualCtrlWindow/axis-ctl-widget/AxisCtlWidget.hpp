#pragma once

#include <eng/sibus/node.hpp>

#include <QWidget>

class AxisCtlItemWidget;
class QVBoxLayout;
class QHBoxLayout;
class manual_engine_set_dlg;

class AxisCtlWidget
    : public QWidget 
    , public eng::sibus::node
{
    QVBoxLayout* vL_;
    std::vector<QHBoxLayout*> hL_;

    std::map<char, AxisCtlItemWidget*> axis_;

    manual_engine_set_dlg *manual_engine_set_dlg_;
    char target_axis_{ '\0' };

    eng::sibus::capture_id_t capture_axis_id_;
    eng::sibus::apply_id_t apply_id_;

    eng::sibus::output_port_id_t oport_axis_;
    eng::sibus::output_wire_id_t owire_;

    // TODO: Исходные значения должны быть true, true
    bool mode_auto_{ false };
    bool mode_rcu_{ false };

public:

    AxisCtlWidget(QWidget*) noexcept;

private:

    void register_on_bus_done() override final;

private:

    void update_axis_ctl_access(bool);

private:

    void grab_axis(char);

private:

    void add_axis(char, std::string_view);

    void show_axis_move_dlg(char) noexcept;

    void update_axis_widgets();

private:

    void apply_axis(char);

    void apply_pos(double);
};
