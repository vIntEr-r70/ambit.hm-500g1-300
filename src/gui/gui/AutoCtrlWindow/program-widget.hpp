#pragma once

#include <QWidget>

class ProgramModel;
class QTableWidget;
class QTableView;
class VerticalScroll;

class program_widget final
    : public QWidget
{
    ProgramModel &model_;

    QTableWidget *thead_;
    QTableView *tbody_;

    VerticalScroll *vscroll_;

public:

    program_widget(QWidget *, ProgramModel &);

public:

    QTableView* tbody() { return tbody_; }

    void rows_count_changed();

private:

    void resizeEvent(QResizeEvent *) override final;

    void showEvent(QShowEvent *) override final;

private:

    void make_scroll(int);
};
