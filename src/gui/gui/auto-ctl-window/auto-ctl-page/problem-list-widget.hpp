#pragma once

#include <QWidget>

#include <unordered_map>

class QTableWidget;

enum class problem_flag
{
    module_access,
    programm,
    cnc,
    cnc_X,
    cnc_Y,
    cnc_Z,
    cnc_V,
    fc,
    pump_fc,
    pump_sp,
    sp_0,
    sp_1,
    sp_2,
};

class problem_list_widget final
    : public QWidget
{
    QTableWidget *table_;

    struct flag_item_t
    {
        int irow{ -1 };
        bool value{ false };
    };

    std::unordered_map<problem_flag, flag_item_t> flags_;

public:

    problem_list_widget(QWidget *);

public:

    void set_problem(problem_flag, bool);

private:

    void add_problem(problem_flag, QString);
};
