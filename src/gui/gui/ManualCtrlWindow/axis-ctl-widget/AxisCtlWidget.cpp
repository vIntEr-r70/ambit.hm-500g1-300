#include "AxisCtlWidget.hpp"
#include "AxisCtlItemWidget.h"
#include "manual-engine-set-dlg/manual-engine-set-dlg.h"

#include <eng/log.hpp>

#include <QVBoxLayout>

#include <algorithm>

AxisCtlWidget::AxisCtlWidget(QWidget *parent) noexcept
    : QWidget(parent)
{
    manual_engine_set_dlg_ = new manual_engine_set_dlg(this);
    manual_engine_set_dlg_->setVisible(false);

    connect(manual_engine_set_dlg_, &manual_engine_set_dlg::axis_ctl_access,
            [this](bool access) { update_axis_ctl_access(access); });

    // connect(manual_engine_set_dlg_, &manual_engine_set_dlg::apply_axis,
    //         [this](char axis) { apply_axis(axis); });

    // connect(manual_engine_set_dlg_, &manual_engine_set_dlg::apply_pos,
    //         [this](double pos) { apply_pos(pos); });


    // connect(manualEngineSetDlg_, SIGNAL(doCalibrate(char)), this, SLOT(onDoCalibrate(char)));
    // connect(manualEngineSetDlg_, SIGNAL(doZero(char)), this, SLOT(engineMakeAsZero(char)));

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    {
        // Резервируем место под виджеты состояния и управления осями
        vL_ = new QVBoxLayout();
        vL_->setSpacing(15);
        vL->addLayout(vL_);
    }

    for (auto axis : { 'X', 'Y', 'Z', 'A', 'B', 'C', 'U', 'V', 'W', 'E' })
    {
        AxisCtlItemWidget* w = new AxisCtlItemWidget(this, axis);

        connect(w, &AxisCtlItemWidget::on_move_to_click,
                [this, axis] { show_axis_move_dlg(axis); });

        connect(w, &AxisCtlItemWidget::axis_view,
                [this, axis](std::string_view name)
                {
                    update_axis_view(axis, name);
                    manual_engine_set_dlg_->set_axis(axis, name);
                });

        connect(w, &AxisCtlItemWidget::axis_speed,
                [this, axis](double value) {
                    manual_engine_set_dlg_->set_axis_speed(axis, value);
                });

        w->setMinimumWidth(240);
        w->setMaximumWidth(240);
        w->setMaximumHeight(110);
        w->setVisible(false);

        axis_[axis] = w;
    }
}

void AxisCtlWidget::update_axis_ctl_access(bool active)
{
    std::ranges::for_each(axis_, [active](auto &pair) {
        pair.second->set_active(active);
    });
}

void AxisCtlWidget::update_axis_view(char axis, std::string_view name)
{
    // Если такой оси еще нету, сначала создаем ее
    if (axis_.find(axis) == axis_.end())
        return;

    AxisCtlItemWidget *w = axis_[axis];

    if (w->isVisibleTo(this) == !name.empty())
        return;

    if (!name.empty())
    {
        QHBoxLayout* hL = hL_.empty() ? nullptr : hL_.back();

        if (hL == nullptr || hL->count() == 4)
        {
            hL_.push_back(new QHBoxLayout());
            vL_->addLayout(hL_.back());
            hL = hL_.back();
            hL->setSpacing(15);
        }

        hL->addWidget(w);
    }
    else
    {
        auto it = std::ranges::find_if(hL_, [w](QHBoxLayout *layout)
        {
            for (int i = 0; i < layout->count(); ++i)
            {
                QLayoutItem* item = layout->itemAt(i);
                if (item && item->widget() == w)
                    return true;
            }
            return false;
        });

        if (it != hL_.end())
            (*it)->removeWidget(w);
    }

    w->setVisible(!name.empty());
}

void AxisCtlWidget::show_axis_move_dlg(char axis) noexcept
{
    // Показываем диалог управления
    manual_engine_set_dlg_->select_axis(axis);
}

