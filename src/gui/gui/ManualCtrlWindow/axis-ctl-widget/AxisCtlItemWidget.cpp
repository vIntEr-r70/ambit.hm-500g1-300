#include "AxisCtlItemWidget.h"
#include "common/axis-status.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QPainter>

#include <Widgets/ValueViewReal.h>

#include <eng/log.hpp>

#include <bitset>

AxisCtlItemWidget::AxisCtlItemWidget(QWidget* parent, char axis, std::string_view name, bool rotation)
    : QWidget(parent)
    , eng::sibus::node(std::format("axis-gui-ctl-{}", axis))
    , axis_(axis)
    , name_(QString::fromStdString(std::string(name)))
    , rotation_(rotation)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString("border-radius: %1px; background-color: white").arg(20));

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 0);
    effect->setColor(Qt::gray);
    effect->setBlurRadius(20);
    setGraphicsEffect(effect);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    {
        lblHeader_ = new QLabel(this);
        lblHeader_->setMinimumHeight(30);
        lblHeader_->setAttribute(Qt::WA_StyledBackground, false);
        lblHeader_->setAlignment(Qt::AlignCenter);
        lblHeader_->setText(name_);
        layout->addWidget(lblHeader_);
    }

    QHBoxLayout *hL = new QHBoxLayout();
    hL->setSpacing(6);
    {
        for (std::size_t i = 0; i < 2; ++i)
        {
            lblLS_[i] = new QFrame(this);
            lblLS_[i]->setFrameStyle(QFrame::HLine);
            lblLS_[i]->setStyleSheet("border: 3px solid #ffffff;");
            hL->addWidget(lblLS_[i]);
        }
    }
    layout->addLayout(hL);

    hL = new QHBoxLayout();
    hL->setContentsMargins(10, 10, 10, 10);
    {
        vvr_pos_ = new ValueViewReal(this, "");
        vvr_pos_->setPostfix(rotation_ ? "°" : "мм");
        vvr_pos_->setValueFontSize(ValueViewReal::H4);
        vvr_pos_->setValueAlignment(Qt::AlignRight);
        vvr_pos_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        vvr_pos_->set_precision(rotation_ ? 4 : 2);
        vvr_pos_->setTitle("Позиция");
        hL->addWidget(vvr_pos_);

        hL->addSpacing(5);

        vvr_speed_ = new ValueViewReal(this, "");
        vvr_speed_->setPostfix(rotation_ ? "об/мин" : "мм/c");
        vvr_speed_->setValueFontSize(ValueViewReal::H4);
        vvr_speed_->setValueAlignment(Qt::AlignRight);
        vvr_speed_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        vvr_speed_->set_precision(rotation_ ? 4 : 2);
        vvr_speed_->setTitle("Скорость");
        hL->addWidget(vvr_speed_);
    }
    layout->addLayout(hL);

    // Канал для управления драйверами
    ctl_ = node::add_output_wire();

    // Обработчик состояния связи с требуемой осью
    node::set_wire_status_handler(ctl_, [this]
    {
        update_axis_view();
    });

    node::add_input_port("position",
        [this](eng::abc::pack const &args)
        {
            double position = eng::abc::get<double>(args);
            vvr_pos_->set_value(position);
            emit axis_position(position);
        });

    node::add_input_port("real-speed",
        [this](eng::abc::pack const &args)
        {
            double speed = eng::abc::get<double>(args);
            if (rotation_) speed /= 6;
            emit axis_real_speed(speed);
        });

    node::add_input_port("set-speed",
        [this](eng::abc::pack const &args)
        {
            double speed = eng::abc::get<double>(args);
            vvr_speed_->set_value(rotation_ ? (speed / 6) : speed);
            emit axis_set_speed(speed);
        });

    node::add_input_port("status",
        [this](eng::abc::pack const &args)
        {
            update_status(eng::abc::get<std::uint8_t>(args));
        });

    update_axis_view();
}

void AxisCtlItemWidget::execute(eng::abc::pack args)
{
    if (node::is_ready(ctl_) || node::is_active(ctl_))
        node::activate(ctl_, std::move(args));
}

void AxisCtlItemWidget::update_axis_view()
{
    bool active = node::is_ready(ctl_);

    static std::array<QColor, 2> const colors[] = {
        { Qt::gray, Qt::white }, { Qt::green, Qt::black }, { Qt::blue, Qt::white }, { Qt::red, Qt::white }
    };

    std::size_t color_scheme_id{ 0 };

    if (active)
        color_scheme_id = 1;

    if (fault_)
        color_scheme_id = 3;

    auto const& clr = colors[color_scheme_id];
    lblHeader_->setStyleSheet(QString("border-bottom-left-radius: 0; border-bottom-right-radius: 0;"
                "background-color: %1; color: %2;").arg(clr[0].name()).arg(clr[1].name()));
}

void AxisCtlItemWidget::update_status(std::uint8_t status)
{
    std::bitset<8> bits(status);

    bool ros = bits.test(ambit::axis_status::ros);
    lblLS_[0]->setStyleSheet(QString("border: 3px solid %1").arg(ros ? "#0000ff" : "#ffffff"));

    bool fos = bits.test(ambit::axis_status::fos);
    lblLS_[1]->setStyleSheet(QString("border: 3px solid %1").arg(fos ? "#0000ff" : "#ffffff"));

    if (fault_ != bits.test(ambit::axis_status::fault))
    {
        fault_ = bits.test(ambit::axis_status::fault);
        update_axis_view();
    }
}

void AxisCtlItemWidget::mousePressEvent(QMouseEvent*)
{
    if (node::is_ready(ctl_)) emit on_move_to_click();
}

// void AxisCtlItemWidget::ls_min_max(std::size_t id, bool v) noexcept
// {
//
//     updateGui();
// }

// void AxisCtlItemWidget::set_sys_ctrl_mode_axis(char v) noexcept
// {
//     ctrlModeAxis_ = v;
//     updateGui();
// }

// void AxisCtlItemWidget::updateGui()
// {
    // uint8_t status = NotActive;
    //
    // switch(sysMode_)
    // {
    // case Core::SysMode::Manual:
    //     if (ctlMode_ == Core::CtlMode::Panel)
    //         status = Active;
    //     else if (ctrlModeAxis_ == axisId_)
    //         status = SelectAsCtrl;
    //     break;
    // }
    //
    // if (Core::ErrorBits::check(error_, Core::ErrorBits::rs422Port) ||
    //     Core::ErrorBits::check(error_, Core::ErrorBits::cncNoConnect))
    // {
    //     status = NotActive;
    // }
    //
    // vvr_speed_->setEnabled(sysMode_ == Core::SysMode::Manual && !std::isnan(speed_));
    // vvr_speed_->set_value(std::isnan(speed_) ? speed_ : UnitsCalc::toSpeed(axis_.muGrads, speed_));
    //
    // float out = UnitsCalc::toPos(axis_.muGrads, pos_);
    // if (axis_.muGrads)
    //     out = out - ((static_cast<int>(out / 360)) * 360);
    // vvr_pos_->set_value(out);
    //
    // setStatus(status);
// }

