#pragma once

#include <QWidget>
#include <QModelIndex>

class ProgramModel;
class AutoParamKeyboard;
class QTableView;
class ValueSetString;
class EditorMessageBox;
class IconButton;

class AutoEditWidget final
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

    AutoEditWidget(QWidget*, ProgramModel&) noexcept;

public:

    void init();

    void set_position(char, float);

signals:

    void make_done();

public slots:

    void tableCellSelect(QModelIndex) noexcept;

    void tableCellClicked(QModelIndex) noexcept; 

private:

    void edit_main_op(std::size_t, std::size_t);

    void edit_pause_op(std::size_t);

    void edit_fc_op(std::size_t);

    void edit_loop_op(std::size_t);

private:

    void need_save(bool) noexcept;

    void make_table_op(TableAc) noexcept;

    void do_exit() noexcept;

    void do_save() noexcept;

private:

    ProgramModel &model_;

    AutoParamKeyboard *kb_;

    ValueSetString *name_;
    ValueSetString *comments_;

    IconButton *btn_save_;

    bool need_save_{ false };

    EditorMessageBox *msg_;

    std::unordered_map<char, float> real_pos_;
};

