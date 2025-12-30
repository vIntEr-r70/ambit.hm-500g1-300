#include "axis-ctl-widget.hpp"

#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QStringView>
#include <QGridLayout>
#include <QTabWidget>

#include <QtWidgets>
#include <Widgets/RoundButton.h>
#include <Widgets/ValueSetReal.h>
#include <Widgets/NumberCalcField.h>
#include <InteractWidgets/InteractWidget.h>
#include <InteractWidgets/KeyboardWidget.h>
#include <Interact.h>

#include <iostream>
#include <qnamespace.h>

axis_ctl_widget::axis_ctl_widget(InteractWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hL = new QHBoxLayout(this);
    hL->setContentsMargins(10, 10, 10, 10);
    hL->setSpacing(30);
    {
        QTableWidget* tw = new QTableWidget(this);
        {
            axis_table_ = tw;
            tw->setFixedWidth(200);
            tw->setFocusPolicy(Qt::NoFocus);
            QFont f(font());
            f.setPointSize(16);
            tw->setFont(f);
            tw->verticalHeader()->hide();
            tw->setEditTriggers(QAbstractItemView::NoEditTriggers);
            tw->setSelectionMode(QAbstractItemView::SingleSelection);
            tw->setSelectionBehavior(QAbstractItemView::SelectRows);
            tw->setAlternatingRowColors(true);
            tw->setColumnCount(2);
            tw->setColumnWidth(1, 50);
            auto hheader = tw->horizontalHeader();
            hheader->hide();
            hheader->setStretchLastSection(false);
            hheader->setSectionResizeMode(QHeaderView::Stretch);
            hheader->setSectionResizeMode(1, QHeaderView::Fixed);
            connect(tw, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)),
                    this, SLOT(on_axis_selected(QTableWidgetItem*, QTableWidgetItem*)));
        }
        hL->addWidget(tw);

        QVBoxLayout *vL = new QVBoxLayout();
        {
            QGridLayout *gL = new QGridLayout();
            {
                gL->addWidget(new QLabel("Текущая позиция:", this), 0, 0);
                gL->addWidget(new QLabel("Текущая скорость:", this), 1, 0);
                gL->addWidget(new QLabel("Максимальная cкорость:", this), 2, 0);

                gL->addWidget(new QLabel("100", this), 0, 1);
                gL->addWidget(new QLabel("200", this), 1, 1);
                gL->addWidget(new QLabel("300", this), 2, 1);
            }
            vL->addLayout(gL);

            tab_ = new QTabWidget(this);
            QFont f(font());
            f.setPointSize(12);
            tab_->setFont(f);
            tab_->setFocusPolicy(Qt::NoFocus);
            {
                QWidget *w = new QWidget(tab_);
                {
                    QVBoxLayout *vL = new QVBoxLayout(w);
                    vL->setContentsMargins(20, 20, 20, 20);
                    {
                        vsr_abs_value_ = new ValueSetReal(w);
                        connect(vsr_abs_value_, &ValueSetReal::onValueChanged, [this, parent]
                        {
                            std::cout << "Interact::dialog" << std::endl;
                            Interact::dialog(parent);
                        });
                        vsr_abs_value_->setValueFontSize(ValueView::H1);
                        vL->addWidget(vsr_abs_value_);

                        QHBoxLayout *hL = new QHBoxLayout();
                        {
                            auto btn = new RoundButton(w);
                            connect(btn, &RoundButton::clicked, [this]
                            {
                                auto axis = get_selected_axis();
                                auto pos = vsr_abs_value_->value();
                                emit axis_command({ axis, "move-to", pos, axis_[axis].speed });
                            });
                            btn->setText("▶");
                            btn->setBgColor(QColor("#29AC39"));
                            btn->setTextColor(Qt::white);
                            btn->setFontSize(20);
                            hL->addWidget(btn);

                            btn = new RoundButton(w);
                            connect(btn, &RoundButton::clicked, [this]
                            {
                                auto axis = get_selected_axis();
                                emit axis_command({ axis, "stop" });
                            });
                            btn->setText("⏹");
                            btn->setBgColor(QColor("#29AC39"));
                            btn->setTextColor(Qt::white);
                            btn->setFontSize(20);
                            hL->addWidget(btn);
                        }
                        vL->addLayout(hL);
                    }
                }
                tab_->addTab(w, "Позиция");

                w = new QWidget(tab_);
                {
                    QVBoxLayout *vL = new QVBoxLayout(w);
                    vL->setContentsMargins(20, 20, 20, 20);
                    {
                        vsr_dx_value_ = new ValueSetReal(w);
                        connect(vsr_dx_value_, &ValueSetReal::onValueChanged, [this, parent]
                        {
                            std::cout << "Interact::dialog" << std::endl;
                            Interact::dialog(parent);
                        });
                        vsr_dx_value_->setValueFontSize(ValueView::H1);
                        vL->addWidget(vsr_dx_value_);

                        QHBoxLayout *hL = new QHBoxLayout();
                        {
                            auto btn = new RoundButton(w);
                            connect(btn, &RoundButton::clicked, [this]
                            {
                                auto axis = get_selected_axis();
                                auto pos = vsr_dx_value_->value();
                                emit axis_command({ axis, "shift", pos, axis_[axis].speed });
                            });
                            btn->setText("▶");
                            btn->setBgColor(QColor("#29AC39"));
                            btn->setTextColor(Qt::white);
                            btn->setFontSize(20);
                            hL->addWidget(btn);

                            btn = new RoundButton(w);
                            connect(btn, &RoundButton::clicked, [this]
                            {
                                auto axis = get_selected_axis();
                                emit axis_command({ axis, "stop" });
                            });
                            btn->setText("⏹");
                            btn->setBgColor(QColor("#29AC39"));
                            btn->setTextColor(Qt::white);
                            btn->setFontSize(20);
                            hL->addWidget(btn);
                        }
                        vL->addLayout(hL);
                    }
                }
                tab_->addTab(w, "Сдвиг");

                w = new QWidget(tab_);
                {
                    QHBoxLayout *hL = new QHBoxLayout(w);
                    {
                        auto btn = new RoundButton(w);
                        connect(btn, &RoundButton::clicked, [this]
                        {
                            auto axis = get_selected_axis();
                            emit axis_command({ axis, "spin", -axis_[axis].speed });
                        });
                        btn->setText("↩");
                        btn->setBgColor(QColor("#29AC39"));
                        btn->setTextColor(Qt::white);
                        btn->setFontSize(20);
                        hL->addWidget(btn);

                        btn = new RoundButton(w);
                        connect(btn, &RoundButton::clicked, [this]
                        {
                            auto axis = get_selected_axis();
                            emit axis_command({ axis, "stop" });
                        });
                        btn->setText("⏹");
                        btn->setBgColor(QColor("#29AC39"));
                        btn->setTextColor(Qt::white);
                        btn->setFontSize(20);
                        hL->addWidget(btn);

                        btn = new RoundButton(w);
                        connect(btn, &RoundButton::clicked, [this]
                        {
                            auto axis = get_selected_axis();
                            emit axis_command({ axis, "spin", axis_[axis].speed });
                        });
                        btn->setText("↪");
                        btn->setBgColor(QColor("#29AC39"));
                        btn->setTextColor(Qt::white);
                        btn->setFontSize(20);
                        hL->addWidget(btn);
                    }
                }
                tab_->addTab(w, "Вращение");
            }
            vL->addWidget(tab_);
        }
        hL->addLayout(vL);
    }

    tab_->setTabVisible(0, false);
    tab_->setTabVisible(1, false);
    tab_->setTabVisible(2, false);
}

// Добавляем или удаляем запись об оси из таблицы
void axis_ctl_widget::set_axis(char axis, std::string_view name)
{
    int rows = axis_table_->rowCount();
    axis_table_->setRowCount(rows + 1);

    QTableWidgetItem* twi = new QTableWidgetItem(QString::fromStdString(std::string(name)));
    axis_table_->setItem(rows, 0, twi);
    twi->setData(Qt::UserRole, axis);

    twi = new QTableWidgetItem();
    axis_table_->setItem(rows, 1, twi);

    axis_[axis].irow = rows;
}

void axis_ctl_widget::select_axis(char axis)
{
    axis_table_->selectRow(axis_[axis].irow);
}

void axis_ctl_widget::set_axis_speed(char axis, double value)
{
    axis_[axis].speed = value;
}

char axis_ctl_widget::get_selected_axis() const
{
    int irow = axis_table_->currentRow();
    if (irow < 0) return '\0';
    QTableWidgetItem* twi = axis_table_->item(irow, 0);
    return twi->data(Qt::UserRole).toChar().toLatin1();
}

void axis_ctl_widget::on_axis_selected(QTableWidgetItem *current, QTableWidgetItem*)
{
    QTableWidgetItem* twi = current ? axis_table_->item(current->row(), 0) : nullptr;
    if (twi != nullptr)
    {
        char axis = twi->data(Qt::UserRole).toChar().toLatin1();

        tab_->setTabVisible(0, true);
        tab_->setTabVisible(1, true);
        tab_->setTabVisible(2, true);
    }
    else
    {
        tab_->setTabVisible(0, false);
        tab_->setTabVisible(1, false);
        tab_->setTabVisible(2, false);
    }
}

