#pragma once

#include <QWidget>

class program_model;
class QTableWidget;
class QTableView;
class VerticalScroll;

class program_widget final
    : public QWidget
{
    program_model &model_;

    QTableWidget *thead_;
    QTableView *tbody_;

    VerticalScroll *vscroll_;

public:

    program_widget(QWidget *, program_model &);

public:

    QTableView* tbody() { return tbody_; }

    void update_table_row_span(std::size_t);

    void update_view();

private:

    void resizeEvent(QResizeEvent *) override final;

    void showEvent(QShowEvent *) override final;

private:

    void make_scroll(int);
};
