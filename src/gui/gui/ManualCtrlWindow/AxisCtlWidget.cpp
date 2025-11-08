#include "AxisCtlWidget.hpp"
#include "AxisCtlItemWidget.h"

#include <QVBoxLayout>

#include <algorithm>

AxisCtlWidget::AxisCtlWidget(QWidget *parent) noexcept
    : QWidget(parent)
    , eng::sibus::node("AXIS-CTL")
{
    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    {
        // Резервируем место под виджеты состояния и управления осями
        vL_ = new QVBoxLayout();
        vL_->setSpacing(15);
        vL->addLayout(vL_);
    }

    node::add_config_listener("axis")
        .on(".{}.name", [this](char axis, eng::abc::pack const &value)
        {
            add_axis(axis, eng::abc::get<std::string_view>(value));
        });

    node::add_output_ports_listener("LC10E.{}.speed",
        [this](char axis, eng::abc::pack const &value) {
            float speed = eng::abc::get<float>(value);
        });

    node::add_output_ports_listener("LC10E.{}.pos",
        [this](char axis, eng::abc::pack const &value) {
            float pos = eng::abc::get<float>(value);
        });

    node::add_output_ports_listener("LC10E.{}.ls",
        [this](char axis, eng::abc::pack const &value) {
            bool min = eng::abc::get<bool>(value, 0);
            bool max = eng::abc::get<bool>(value, 1);
        });
}

void AxisCtlWidget::add_axis(char axis, std::string_view name)
{
    // Если такой оси еще нету, сначала создаем ее
    if (axis_.find(axis) == axis_.end())
    {
        AxisCtlItemWidget* w = new AxisCtlItemWidget(this, axis);
        connect(w, SIGNAL(onMoveToClick(char)), this, SLOT(onAxisWidgetMoveTo(char)));

        w->setMinimumWidth(240);
        w->setMaximumWidth(240);
        w->setMaximumHeight(110);
        w->setVisible(false);

        axis_[axis] = w;
    }

    axis_[axis]->update_name(name);

    add_axis(axis, !name.empty());
}

void AxisCtlWidget::add_axis(char axis, bool append) noexcept
{
    AxisCtlItemWidget *w = axis_[axis];

    if (w->isVisibleTo(this) == append)
        return;

    if (append)
    {
        QHBoxLayout* hL = hL_.empty() ? nullptr : hL_.back();

        if (hL == nullptr || hL->count() == 4)
        {
            hL_.push_back(new QHBoxLayout());
            vL_->addLayout(hL_.back());
            hL = hL_.back();
            hL->setSpacing(15);
        }

        w->apply_self_axis();

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

    w->setVisible(append);
}

// void ManualCtrlWindow::nf_sys_error(unsigned int v) noexcept
// {
//     for (auto& item : axis_)
//         item.second->set_sys_error(v);
// }

// void ManualCtrlWindow::nf_sys_mode(unsigned char v) noexcept
// {
//     for (auto& item : axis_)
//         item.second->set_sys_mode(v);
// }

// void ManualCtrlWindow::nf_sys_ctrl(unsigned char v) noexcept
// {
//     for (auto& item : axis_)
//         item.second->set_sys_ctrl(v);
// }

// void ManualCtrlWindow::nf_sys_ctrl_mode_axis(char v) noexcept
// {
//     for (auto& item : axis_)
//         item.second->set_sys_ctrl_mode_axis(v);
// }

// void ManualCtrlWindow::apply_axis_cfg() noexcept
// {
//     std::for_each(axis_cfg_.begin(), axis_cfg_.end(), [this](auto &&it)
//     {
//         if (!it.second.use())
//             return;
//         addAxis(it.first);
//     });
// }


