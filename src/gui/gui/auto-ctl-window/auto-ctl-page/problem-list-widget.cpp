#include "problem-list-widget.hpp"

#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>

problem_list_widget::problem_list_widget(QWidget *parent)
    : QWidget(parent)
{
    auto hL = new QHBoxLayout(this);
    hL->setContentsMargins(0, 0, 0, 0);
    {
        table_ = new QTableWidget(this);
        table_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table_->setFrameStyle(QFrame::Box);
        table_->setSelectionMode(QAbstractItemView::NoSelection);
        table_->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        // table_->setAlternatingRowColors(true);
        // table_->setFont(f);
        table_->verticalHeader()->hide();
        table_->setColumnCount(2);
        auto hheader = table_->horizontalHeader();
        hheader->hide();
        hheader->setSectionResizeMode(0, QHeaderView::Stretch);
        hheader->setSectionResizeMode(1, QHeaderView::Fixed);
        table_->setColumnWidth(1, 200);
        table_->setSortingEnabled(false);
        hL->addWidget(table_);
    }

    // add_problem(problem_flag::module_access, "Система контроля автоматического режима");
    // add_problem(problem_flag::cnc, "Система управления движением");
    // add_problem(problem_flag::programm, "Программа");
    // add_problem(problem_flag::cnc_X, "Драйвер движения X");
    // add_problem(problem_flag::cnc_Y, "Драйвер движения Y");
    // add_problem(problem_flag::cnc_Z, "Драйвер движения Z");
    // add_problem(problem_flag::cnc_V, "Драйвер движения V");
    // add_problem(problem_flag::fc, "Преобразователь частоты");
    // add_problem(problem_flag::pump_fc, "Насос охлаждения преобразователя частоты");
    // add_problem(problem_flag::pump_sp, "Насос спрейеров");
    // add_problem(problem_flag::sp_0, "Спрейер №1");
    // add_problem(problem_flag::sp_1, "Спрейер №2");
    // add_problem(problem_flag::sp_2, "Спрейер №3");
}

void problem_list_widget::clear()
{
    while (table_->rowCount())
        table_->removeRow(table_->rowCount() - 1);
}

void problem_list_widget::append(std::string_view emsg)
{
    int row = table_->rowCount();
    table_->insertRow(row);

    auto item = new QTableWidgetItem(QString::fromUtf8(emsg.data(), emsg.length()));
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table_->setItem(row, 0, item);

    item = new QTableWidgetItem("");
    item->setTextAlignment(Qt::AlignCenter);
    table_->setItem(row, 1, item);
}

// void problem_list_widget::add_problem(problem_flag value, QString title)
// {
//     int row = table_->rowCount();
//     table_->insertRow(row);
//
//     auto item = new QTableWidgetItem(std::move(title));
//     item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//     table_->setItem(row, 0, item);
//
//     item = new QTableWidgetItem("");
//     item->setTextAlignment(Qt::AlignCenter);
//     table_->setItem(row, 1, item);
//
//     flags_[value] = { row, false };
// }

// void problem_list_widget::set_problem(problem_flag flag, bool value)
// {
//     auto &f = flags_[flag];
//     f.value = value;
//
//     if (f.irow < 0) return;
//
//     auto item = table_->item(f.irow, 0);
//     item->setBackground(value ? QColor("#f7bbbb") : QColor("#bbf7bb"));
//
//     item = table_->item(f.irow, 1);
//     item->setText(value ? "ОШИБКА" : "НОРМА");
//     item->setBackground(value ? QColor("#f7bbbb") : QColor("#bbf7bb"));
//     item->setForeground(value ? Qt::red : Qt::green);
// }

