#include "AxisCtlWidget.hpp"
#include "AxisCtlItemWidget.h"
#include "manual-engine-set-dlg/manual-engine-set-dlg.h"

#include <eng/log.hpp>
#include <eng/json.hpp>

#include <QVBoxLayout>

#include <filesystem>

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

    char const *LIAEM_RO_PATH = std::getenv("LIAEM_RO_PATH");
    if (LIAEM_RO_PATH == nullptr)
        return;

    std::filesystem::path path(LIAEM_RO_PATH);
    path /= "axis.json";

    try
    {
        QHBoxLayout* hL = nullptr;

        eng::json::array cfg(path);
        cfg.for_each([this, vL, &hL](std::size_t, eng::json::value v)
        {
            eng::json::object obj(v);

            char axis = obj.get_sv("axis", "")[0];
            bool rotation = obj.get<bool>("rotation", false);
            auto name = obj.get_sv("name", "");

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

            connect(w, &AxisCtlItemWidget::axis_state,
                    [this, axis](double position, double speed) {
                        manual_engine_set_dlg_->set_axis_state(axis, position, speed);
                    });

            connect(w, &AxisCtlItemWidget::axis_speed,
                    [this, axis](double value) {
                        manual_engine_set_dlg_->set_axis_max_speed(axis, value);
                    });

            w->setMinimumWidth(240);
            w->setMaximumWidth(240);
            w->setMaximumHeight(110);

            axis_[axis] = w;
        });
    }
    catch(std::exception const &e)
    {
        axis_.clear();
        return;
    }
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

