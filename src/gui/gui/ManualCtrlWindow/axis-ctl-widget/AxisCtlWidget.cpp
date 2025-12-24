#include "AxisCtlWidget.hpp"
#include "AxisCtlItemWidget.h"
#include "eng/utils.hpp"
#include "manual-engine-set-dlg.h"

#include <Interact.h>

#include <QVBoxLayout>

#include <algorithm>

AxisCtlWidget::AxisCtlWidget(QWidget *parent) noexcept
    : QWidget(parent)
    , eng::sibus::node("AXIS-CTL")
{
    manual_engine_set_dlg_ = new manual_engine_set_dlg(this);
    manual_engine_set_dlg_->setVisible(false);

    connect(manual_engine_set_dlg_, &manual_engine_set_dlg::apply_axis,
            [this](char axis) { apply_axis(axis); });

    connect(manual_engine_set_dlg_, &manual_engine_set_dlg::apply_pos,
            [this](double pos) { apply_pos(pos); });


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

    node::add_config_listener("axis")
        .on(".{}.name", [this](char axis, eng::abc::pack const &value)
        {
            add_axis(axis, eng::abc::get<std::string_view>(value));
        });

    for (auto axis : { 'X', 'Y', 'Z', 'V' })
    {
        node::add_input_port(std::format("{}", axis),
            [this, axis](eng::abc::pack const &args)
            {
                double position = eng::abc::get<double>(args, 0);
                double speed = eng::abc::get<double>(args, 1);

                auto it = axis_.find(axis);
                if (it != axis_.end())
                {
                    it->second->update_current_position(position);
                    it->second->update_current_speed(speed);
                }
            });
    }

    // Текущая управляемая ось
    oport_axis_ = node::add_output_port("axis");

    // Канал для управления драйверами
    owire_ = node::add_output_wire();
    // Успешный ответ на вызов с аргументами
    node::set_wire_signal_done_handler(owire_, [](eng::abc::pack args) {
        std::println("node::set_wire_signal_done_handler: {}", eng::utils::to_hex(args.to_span()));
    });
    // Системная ошибка на вызов
    node::set_wire_signal_failed_handler(owire_, [](std::string_view emsg) {
        std::println("node::set_wire_signal_failed_handler: {}", emsg);
    });

    // Обработчик состояния связи с требуемой осью
    node::set_wire_online_handler(owire_, [this]
    {
        std::println("node::set_wire_online_handler");
        manual_engine_set_dlg_->set_axis_name(axis_[target_axis_]->name());
    });
    node::set_wire_offline_handler(owire_, [this]
    {
        std::println("node::set_wire_offline_handler");
        manual_engine_set_dlg_->set_axis_name("");
    });
}

// // Захват либо удачен сразу либо будет ожидать на шине пока не станет удачен
// // Захвачено может быть несколько ресурсов
// void AxisCtlWidget::capture_success(eng::sibus::capture_id_t)
// {
//     // Ось захвачена для управления
// }


// Либо мы либо шина либо клиент инициировал разрыв
// void AxisCtlWidget::capture_done(eng::sibus::capture_id_t)
// {
//     // Не удалось захватить ось для управления
//     manual_engine_set_dlg_->set_axis_name("");
//     // Больше он не актуален для нас
//     capture_axis_id_.reset();
// }

void AxisCtlWidget::add_axis(char axis, std::string_view name)
{
    // Если такой оси еще нету, сначала создаем ее
    if (axis_.find(axis) == axis_.end())
    {
        AxisCtlItemWidget* w = new AxisCtlItemWidget(this);
        connect(w, &AxisCtlItemWidget::on_move_to_click,
                [this, axis] { show_axis_move_dlg(axis); });

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

void AxisCtlWidget::show_axis_move_dlg(char axis) noexcept
{
    // Активируем управление осью
    apply_axis(axis);

    // Показываем диалог управления
    Interact::dialog(manual_engine_set_dlg_);

    // manualEngineSetDlg_->show(axis);

    // rpc_.call("set", { "cnc", "axis-stop-all", {} })
    //     .done([this](nlohmann::json const&)
    //     {
    //         // Показываем диалог
           // Interact::dialog(manualEngineSetDlg_);
    //     })
    //     .error([this](std::string_view emsg)
    //     {
    //         aem::log::error("ManualCtrlWindow::onAxisWidgetClick: axis-stop-all: {}", emsg);
    //     });
}

// Захватываем ось для работы
void AxisCtlWidget::apply_axis(char axis)
{
    // Выставляем желаемую для контроля ось
    // Ждем появления связи с драйвером выбранной оси
    node::set_port_value(oport_axis_, { axis });
    target_axis_ = axis;

    // // Если в данный момент обрабатывается команда на захват оси, отменяем данную команду
    // if (capture_axis_id_)
    //     node::release_capture(capture_axis_id_);
    //
    // // Запоминаем, какую ось хотим захватить
    // target_axis_ = axis;
    //
    // // Отправляем команду на захват ресурса
    // capture_axis_id_ = node::capture({ axis });
}

// Отправляем новую позицию двигателю
void AxisCtlWidget::apply_pos(double pos)
{
    std::println("AxisCtlWidget::apply_pos: pos = {}", pos);
    node::send_wire_signal(owire_, { 'P', 'a', pos });
    // Отправляем команду на задание позиции для захваченной оси
    // eng::abc::pack args{ *target_axis_, "pos", pos };
    // apply_id_ = node::apply(capture_axis_id_, std::move(args))
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


