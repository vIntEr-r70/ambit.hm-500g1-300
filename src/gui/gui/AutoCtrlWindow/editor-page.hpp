#pragma once

#include <QWidget>

class ProgramModel;
class AutoParamKeyboard;
class editor_page_header_widget;
class program_widget;

class editor_page final
    : public QWidget
{
    Q_OBJECT

    ProgramModel &model_;
    program_widget *program_widget_;

    editor_page_header_widget *header_;
    AutoParamKeyboard *kb_;

    std::unordered_map<char, float> real_pos_;

public:

    editor_page(QWidget *, ProgramModel &);

public:

    void init(QString const &);

private:

    void table_cell_select(QModelIndex);

signals:

    void make_done();
};
