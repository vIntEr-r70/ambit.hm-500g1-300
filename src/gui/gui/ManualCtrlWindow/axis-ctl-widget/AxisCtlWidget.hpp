#pragma once

#include <QWidget>

class AxisCtlItemWidget;
class QVBoxLayout;
class QHBoxLayout;
class manual_engine_set_dlg;

class AxisCtlWidget
    : public QWidget 
{
    QVBoxLayout* vL_;
    std::vector<QHBoxLayout*> hL_;

    std::map<char, AxisCtlItemWidget*> axis_;

    manual_engine_set_dlg *manual_engine_set_dlg_;

public:

    AxisCtlWidget(QWidget*) noexcept;

private:

    void update_axis_view(char, std::string_view);

    void update_axis_ctl_access(bool);

    void show_axis_move_dlg(char) noexcept;
};
