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

    QTabWidget *tab_;

    struct axis_t
    {
        int irow{ -1 };
        double speed{ 0.0 };
    };
    std::unordered_map<char, axis_t> axis_;

public:

    axis_ctl_widget(InteractWidget *);

public:

    void set_axis(char, std::string_view);

    void set_axis_speed(char, double);

    void select_axis(char);

private:

    char get_selected_axis() const;

private slots:

    void on_axis_selected(QTableWidgetItem*, QTableWidgetItem*);

signals:

    void axis_command(eng::abc::pack);
};
