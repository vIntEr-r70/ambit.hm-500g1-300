#pragma once

#include <eng/abc/pack.hpp>

#include <QWidget>

class QTableWidget;
class QTableWidgetItem;
class InteractWidget;
class ValueSetReal;
class RoundButton;
class QTabWidget;

class axis_ctl_widget final
    : public QWidget
{
    Q_OBJECT

    QTableWidget *axis_table_;

    ValueSetReal *vsr_abs_value_;
    ValueSetReal *vsr_dx_value_;

    // RoundButton *btn_absolute_move_;
    // bool absolute_move_{ true };

    QTabWidget *tab_;

public:

    axis_ctl_widget(InteractWidget *);

private:

    void execute_axis_command(std::string_view, double);

    void execute_axis_command(std::string_view);

    char get_selected_axis() const;

public:

    void set_axis(char, std::string_view);

private slots:

    void on_axis_selected(QTableWidgetItem*, QTableWidgetItem*);

signals:

    void axis_command(eng::abc::pack);
};
