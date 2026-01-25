#pragma once

// #include "func-map.h"

#include <QStackedWidget>

// class AutoCtrlWidget;
// class AutoEditWidget;
class ProgramModel;
// class QStackedWidget;
// class QTableView;
// class QTableWidget;

class auto_ctl_page;
class editor_page;
class common_page;

class auto_mode_window final
    : public QStackedWidget
{
    ProgramModel *model_;

    common_page *common_page_;
    auto_ctl_page *auto_ctl_page_;
    editor_page *editor_page_;

public:

    auto_mode_window(QWidget *) noexcept;

public:

    // void apply_axis_cfg() noexcept;
    //
    // void nf_sys_mode(unsigned char) noexcept;
    //
    // void set_guid(int);

private:

    // void resizeEvent(QResizeEvent*) noexcept override final;

private:

    // void do_start();
    //
    // void do_stop();
    //
    // void do_load_program();
    //
    // void make_scroll(int);

private:

    // QStackedWidget *stack_;
    //
    // AutoCtrlWidget *autoCtrlWidget_;
    // AutoEditWidget *autoEditWidget_;
    //
    // QTableWidget *pHdrW_;
    // QTableView *pTblW_;
    //
    // ProgramModel *model_;
    //
    // func_multi_map sys_key_map_;
};

