#include "manual-engine-set-dlg.h"
#include "axis-ctl-widget.hpp"

#include <Widgets/RoundButton.h>
#include <Widgets/NumberCalcField.h>
#include <InteractWidgets/KeyboardWidget.h>
#include <Interact.h>
#include <UnitsCalc.h>

#include <eng/log.hpp>

#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QDoubleValidator>
#include <QStackedWidget>

manual_engine_set_dlg::manual_engine_set_dlg(QWidget* parent)
    : InteractWidget(parent)
{
    setObjectName("manual_engine_set_dlg");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#manual_engine_set_dlg { border-radius: 20px; background-color: white }");

    setFixedWidth(550);

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    {
        axis_ctl_ = new axis_ctl_widget(this);
        connect(axis_ctl_, &axis_ctl_widget::axis_command,
            [this](char axis, eng::abc::pack args) {
                emit axis_command(axis, std::move(args));
            });
        vL->addWidget(axis_ctl_);

        QHBoxLayout *hL = new QHBoxLayout();
        vL->setContentsMargins(10, 10, 10, 10);
        {
            RoundButton *btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this]
            {
                InteractWidget::hide();
            });
            btn->setIcon(":/kbd.dlg.cancel");
            btn->setBgColor("#E55056");
            btn->setMinimumWidth(90);
            hL->addWidget(btn);

            hL->addStretch();
        }
        vL->addLayout(hL);
    }

    // Канал для управления драйверами
    // owire_ = node::add_output_wire();

    // Успешный ответ на вызов с аргументами
    // node::set_wire_response_handler(owire_, [](bool success, eng::abc::pack args)
    // {
    //     if (success)
    //         return;
    //     // Системная ошибка на вызов
    //     eng::log::error("manual_engine_set_dlg: {}", eng::abc::get_sv(args));
    // });
    // // Обработчик состояния связи с требуемой осью
    // node::set_wire_link_handler(owire_, [this](bool linked) {
    //     emit axis_ctl_access(linked);
    // });
}

// void manual_engine_set_dlg::register_on_bus_done()
// {
//     node::wire_activate(owire_);
// }

void manual_engine_set_dlg::add_axis(char axis, std::string_view name, bool rotation)
{
    axis_ctl_->set_axis(axis, name);
}

void manual_engine_set_dlg::select_axis(char axis)
{
    axis_ctl_->select_axis(axis);
    Interact::dialog(this);
}

void manual_engine_set_dlg::set_axis_max_speed(char axis, double value)
{
    axis_ctl_->set_axis_max_speed(axis, value);
}

void manual_engine_set_dlg::set_axis_position(char axis, double position)
{
    axis_ctl_->set_axis_position(axis, position);
}

void manual_engine_set_dlg::set_axis_real_speed(char axis, double speed)
{
    axis_ctl_->set_axis_real_speed(axis, speed);
}
