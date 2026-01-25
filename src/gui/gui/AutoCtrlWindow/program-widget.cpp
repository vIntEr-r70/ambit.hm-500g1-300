#include "program-widget.hpp"
#include "ProgramModel.h"
#include "ProgramModelHeader.h"

#include <Widgets/VerticalScroll.h>

#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>

#include <eng/log.hpp>

program_widget::program_widget(QWidget *parent, ProgramModel &model)
    : QWidget(parent)
    , model_(model)
{
    QHBoxLayout *hL = new QHBoxLayout(this);
    hL->setContentsMargins(0, 0, 0, 0);
    hL->setSpacing(0);
    {
        QVBoxLayout *vL = new QVBoxLayout();
        {
            thead_ = new QTableWidget(this);
            thead_->setStyleSheet("font-size: 9pt; color: #ffffff; background-color: #696969; font-weight: 500");
            thead_->verticalHeader()->hide();
            thead_->horizontalHeader()->hide();
            thead_->setFocusPolicy(Qt::NoFocus);
            thead_->setSelectionMode(QAbstractItemView::NoSelection);
            thead_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            thead_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            thead_->setFixedHeight(75 + 7);
            vL->addWidget(thead_);

            tbody_ = new QTableView(this);
            tbody_->setStyleSheet("font-size: 9pt; color: #696969; font-weight: 500");
            tbody_->verticalHeader()->hide();
            tbody_->horizontalHeader()->hide();
            connect(tbody_->verticalScrollBar(), &QScrollBar::rangeChanged,
                [this](int min, int max) { vscroll_->change_range(min, max); });
            connect(tbody_->verticalScrollBar(), &QScrollBar::valueChanged,
                [this](int value) { vscroll_->change_value(value); });
            tbody_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            tbody_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            tbody_->setSelectionMode(QAbstractItemView::NoSelection);
            tbody_->setSortingEnabled(false);
            tbody_->setFocusPolicy(Qt::NoFocus);
            tbody_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
            tbody_->setModel(&model_);
            vL->addWidget(tbody_);
        }
        hL->addLayout(vL);

        hL->addSpacing(5);

        vscroll_ = new VerticalScroll(this);
        connect(vscroll_, &VerticalScroll::move, [this](int shift) { make_scroll(shift); });
        hL->addWidget(vscroll_);
    }
}

void program_widget::rows_count_changed()
{
    // Это заставляем таблицу пересчитать размер полосы прокрутки
    tbody_->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    tbody_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    vscroll_->setVisible(tbody_->verticalScrollBar()->maximum() > 0);

    int ww = width() - (vscroll_->isVisible() ? vscroll_->width() : 0);

    ProgramModelHeader::create_header(model_.prog(), *thead_, ww);
    for (std::size_t i = 0; i < thead_->columnCount(); ++i)
        tbody_->setColumnWidth(i, thead_->columnWidth(i));
}

void program_widget::resizeEvent(QResizeEvent *)
{
    rows_count_changed();
}

void program_widget::showEvent(QShowEvent *)
{
    rows_count_changed();
}

void program_widget::make_scroll(int shift)
{
    auto bar = tbody_->verticalScrollBar();
    int pos = bar->sliderPosition();
    bar->setSliderPosition(pos + shift);
}

