#include "AxisCtlWidget.hpp"
#include "AxisCtlItemWidget.h"
#include "manual-engine-set-dlg/manual-engine-set-dlg.h"
#include "common/load-axis-list.hpp"

#include <eng/log.hpp>

#include <QVBoxLayout>

AxisCtlWidget::AxisCtlWidget(QWidget *parent) noexcept
    : QWidget(parent)
{
    manual_engine_set_dlg_ = new manual_engine_set_dlg(this);
    manual_engine_set_dlg_->setVisible(false);

    connect(manual_engine_set_dlg_, &manual_engine_set_dlg::axis_command,
            [this](char axis, eng::abc::pack args) {
                axis_[axis]->execute(std::move(args));
            });

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    {
        // Резервируем место под виджеты состояния и управления осями
        vL_ = new QVBoxLayout();
        vL_->setSpacing(15);
        vL->addLayout(vL_);
    }

    QHBoxLayout* hL = nullptr;

    ambit::load_axis_list([this, &hL](char axis, std::string_view name, bool rotation)
    {
        manual_engine_set_dlg_->add_axis(axis, name, rotation);

        AxisCtlItemWidget* w = new AxisCtlItemWidget(this, axis, name, rotation);

        if (hL == nullptr || hL->count() == 4)
        {
            hL = new QHBoxLayout();
            hL->setSpacing(15);
            vL_->addLayout(hL);
        }
        hL->addWidget(w);

        connect(w, &AxisCtlItemWidget::on_move_to_click,
                [this, axis] { show_axis_move_dlg(axis); });

        connect(w, &AxisCtlItemWidget::axis_real_speed,
                [this, axis](double speed) {
                    manual_engine_set_dlg_->set_axis_real_speed(axis, speed);
                });

        connect(w, &AxisCtlItemWidget::axis_position,
                [this, axis](double position) {
                    manual_engine_set_dlg_->set_axis_position(axis, position);
                });

        connect(w, &AxisCtlItemWidget::axis_set_speed,
                [this, axis](double value) {
                    manual_engine_set_dlg_->set_axis_max_speed(axis, value);
                });

        w->setMinimumWidth(240);
        w->setMaximumWidth(240);
        w->setMaximumHeight(110);

        axis_[axis] = w;
    });
}

void AxisCtlWidget::update_axis_ctl_access(bool active)
{
    // std::ranges::for_each(axis_, [active](auto &pair) {
    //     pair.second->set_active(active);
    // });
}

void AxisCtlWidget::show_axis_move_dlg(char axis) noexcept
{
    // Показываем диалог управления
    manual_engine_set_dlg_->select_axis(axis);
}

