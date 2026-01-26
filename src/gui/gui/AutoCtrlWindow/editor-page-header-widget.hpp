#pragma once

#include <QWidget>
#include <QModelIndex>

class ProgramModel;
class QTableView;
class ValueSetString;
class EditorMessageBox;
class IconButton;

class editor_page_header_widget final
    : public QWidget
{
    Q_OBJECT

    enum TableAc
    {
        AddMainOp = 0,
        AddRelMainOp,
        AddPauseOp,
        AddGoToOp,
        AddTimedFCOp,
        AddCenterOp,
        DeleteOp
    };

public:

    editor_page_header_widget(QWidget *, ProgramModel &) noexcept;

public:

    void init();

    void need_save(bool) noexcept;

signals:

    void make_done();

    void rows_count_changed(bool);

private:

    void make_table_op(TableAc) noexcept;

    void do_exit() noexcept;

    void do_save() noexcept;

private:

    ProgramModel &model_;

    ValueSetString *name_;
    ValueSetString *comments_;

    IconButton *btn_save_;

    bool need_save_{ false };

    EditorMessageBox *msg_;
};

