#pragma once

#include <QWidget>

class program_model_editor;
class QTableView;
class ValueSetString;
class IconButton;

class editor_page_header_widget final
    : public QWidget
{
    Q_OBJECT

public:

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

    editor_page_header_widget(QWidget *) noexcept;

public:

    void need_save(bool);

    bool need_save() const noexcept;

    void set_program_info(QString const &, QString const &);

    QString const &name() const noexcept;

    QString const &comments() const noexcept;

signals:

    void do_save();

    void do_exit();

    void do_play();

    void make_table_op(TableAc);

private:

    ValueSetString *name_;
    ValueSetString *comments_;

    IconButton *btn_exit_;
    IconButton *btn_save_;
    IconButton *btn_play_;
};

