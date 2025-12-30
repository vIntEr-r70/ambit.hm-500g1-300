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
    , eng::sibus::node("axis-gui-ctl")
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
            [this](eng::abc::pack args) {
                node::send_wire_signal(owire_, std::move(args));
            });
        vL->addWidget(axis_ctl_);

        QHBoxLayout *hL = new QHBoxLayout();
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
    owire_ = node::add_output_wire();

    // Успешный ответ на вызов с аргументами
    node::set_wire_signal_done_handler(owire_, [](eng::abc::pack) {
    });
    // Системная ошибка на вызов
    node::set_wire_signal_failed_handler(owire_, [](std::string_view emsg) {
        eng::log::error("manual_engine_set_dlg: {}", emsg);
    });

    // Обработчик состояния связи с требуемой осью
    node::set_wire_online_handler(owire_, [this] {
        emit axis_ctl_access(true);
    });
    node::set_wire_offline_handler(owire_, [this] {
        emit axis_ctl_access(false);
    });
}

void manual_engine_set_dlg::register_on_bus_done()
{
    node::wire_activate(owire_);
}

void manual_engine_set_dlg::set_axis(char axis, std::string_view name)
{
    axis_ctl_->set_axis(axis, name);
}

void manual_engine_set_dlg::select_axis(char axis)
{
    axis_ctl_->select_axis(axis);
    Interact::dialog(this);
}

void manual_engine_set_dlg::set_axis_speed(char axis, double value)
{
    axis_ctl_->set_axis_speed(axis, value);
}

