#pragma once

#include <QWidget>

class ProgramModel;
class AutoParamKeyboard;
class editor_page_header_widget;

class editor_page final
    : public QWidget
{
    ProgramModel &model_;

    editor_page_header_widget *header_;
    AutoParamKeyboard *kb_;

    std::unordered_map<char, float> real_pos_;

public:

    editor_page(QWidget *, ProgramModel &);

private:

    void table_cell_select(QModelIndex);
};
