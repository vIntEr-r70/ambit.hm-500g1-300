#pragma once

#include <eng/sibus/node.hpp>

#include <QWidget>

class ProgramModel;
class AutoParamKeyboard;
class editor_page_header_widget;
class program_widget;

class editor_page final
    : public QWidget
    , public eng::sibus::node
{
    Q_OBJECT

    ProgramModel &model_;
    program_widget *program_widget_;

    editor_page_header_widget *header_;
    AutoParamKeyboard *kb_;

    std::unordered_map<char, double> positions_;

public:

    editor_page(QWidget *, ProgramModel &);

signals:

    void make_done();

public:

    void init(QString const &);

private:

    void table_cell_select(QModelIndex);
};
