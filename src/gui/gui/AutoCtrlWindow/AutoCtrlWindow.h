#pragma once

#include "func-map.h"

#include <QWidget>

class AutoCtrlWidget;
class AutoEditWidget;
class ProgramModel;
class QStackedWidget;
class QTableView;
class QTableWidget;

namespace we {
    class axis_cfg;
}

class AutoCtrlWindow
    : public QWidget 
{
public:

    AutoCtrlWindow(QWidget*, we::axis_cfg const &) noexcept;

public:

    void apply_axis_cfg() noexcept;

    void nf_sys_mode(unsigned char) noexcept;

    void set_guid(int);

private:

    void resizeEvent(QResizeEvent*) noexcept override final;

private:

    void do_start();

    void do_stop();

    void do_load_program();

    void make_scroll(int);

private:

    we::axis_cfg const &axis_cfg_;

    QStackedWidget *stack_;

    AutoCtrlWidget *autoCtrlWidget_;
    AutoEditWidget *autoEditWidget_;

    QTableWidget *pHdrW_;
    QTableView *pTblW_;

    ProgramModel *model_;

    func_multi_map sys_key_map_;
};

