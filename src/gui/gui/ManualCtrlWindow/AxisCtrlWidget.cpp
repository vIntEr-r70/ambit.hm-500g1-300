#include "AxisCtrlWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QPainter>

#include <Widgets/ValueViewReal.h>
#include "common/Defs.h"

#include <axis-cfg.h>
#include <UnitsCalc.h>

#include <aem/log.h>

#define ROUND_RADIUS 20

AxisCtrlWidget::AxisCtrlWidget(QWidget* parent, char axisId, we::axis_desc const &axis)
    : QWidget(parent)
    , axisId_(axisId)
    , axis_(axis)
{
	setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString("border-radius: %1px; background-color: white").arg(ROUND_RADIUS));

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 0);
    effect->setColor(Qt::gray);
    effect->setBlurRadius(ROUND_RADIUS);
    setGraphicsEffect(effect);

    headClr_ = {{ { Qt::gray, Qt::white }, { Qt::green, Qt::black }, { Qt::blue, Qt::white } }};

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    {
        lblHeader_ = new QLabel(this);
        lblHeader_->setMinimumHeight(30);
	    lblHeader_->setAttribute(Qt::WA_StyledBackground, false);
        lblHeader_->setAlignment(Qt::AlignCenter);
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
        vvr_pos_->setValueFontSize(ValueViewReal::H4);
        vvr_pos_->setValueAlignment(Qt::AlignRight);
        vvr_pos_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        vvr_pos_->set_precision(2);
        vvr_pos_->setTitle("Позиция");
        hL->addWidget(vvr_pos_);

        hL->addSpacing(5);

        vvr_speed_ = new ValueViewReal(this, "");
        vvr_speed_->setValueFontSize(ValueViewReal::H4);
        vvr_speed_->setValueAlignment(Qt::AlignRight);
        vvr_speed_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        vvr_speed_->set_precision(2);
        vvr_speed_->setTitle("Скорость");
        hL->addWidget(vvr_speed_);
    }
    layout->addLayout(hL);

    setStatus(NotActive);
}

void AxisCtrlWidget::mousePressEvent(QMouseEvent*)
{
    if (status_ == Active)
        emit onMoveToClick(axisId_);
}

void AxisCtrlWidget::apply_self_axis() noexcept
{
    lblHeader_->setText(axis_.name);
    vvr_pos_->setPostfix(axis_.muGrads ? "°" : "мм");
    // vvr_pos_->set_precision(axis_.muGrads ? 5 : 2);
    vvr_pos_->set_precision(axis_.muGrads ? 2 : 2);
    vvr_speed_->setPostfix(axis_.muGrads ? "об/мин" : "мм/c");

    updateGui();
}

void AxisCtrlWidget::nf_speed(float v) noexcept
{
    speed_ = v;
    updateGui();
}

void AxisCtrlWidget::ls_min_max(std::size_t id, bool v) noexcept
{
    ls_[id] = v;
    lblLS_[id]->setStyleSheet(QString("border: 3px solid %1").arg(v ? "#0000ff" : "#ffffff"));

    updateGui();
}

void AxisCtrlWidget::nf_pos(float v) noexcept
{
    pos_ = v;
    updateGui();
}

void AxisCtrlWidget::set_sys_mode(unsigned char v) noexcept
{
    sysMode_ = v;
    updateGui();
}

void AxisCtrlWidget::set_sys_ctrl(unsigned char v) noexcept
{
    ctlMode_ = v;
    updateGui();
}

void AxisCtrlWidget::set_sys_error(unsigned int v) noexcept
{
    error_ = v;
    updateGui();
}

void AxisCtrlWidget::set_sys_ctrl_mode_axis(char v) noexcept
{
    ctrlModeAxis_ = v;
    updateGui();
}

void AxisCtrlWidget::setStatus(aem::uint8 status)
{
    status_ = status;

    auto const& clr = headClr_[status];
    lblHeader_->setStyleSheet(QString("border-bottom-left-radius: 0; border-bottom-right-radius: 0;"
                "background-color: %1; color: %2;").arg(clr[0].name()).arg(clr[1].name()));
}

void AxisCtrlWidget::updateGui()
{
    uint8_t status = NotActive;

    switch(sysMode_)
    {
    case Core::SysMode::Manual:
        if (ctlMode_ == Core::CtlMode::Panel)
            status = Active;
        else if (ctrlModeAxis_ == axisId_)
            status = SelectAsCtrl;
        break;
    }

    if (Core::ErrorBits::check(error_, Core::ErrorBits::rs422Port) ||
        Core::ErrorBits::check(error_, Core::ErrorBits::cncNoConnect))
    {
        status = NotActive;
    }
    
    vvr_speed_->setEnabled(sysMode_ == Core::SysMode::Manual && !std::isnan(speed_));
    vvr_speed_->set_value(std::isnan(speed_) ? speed_ : UnitsCalc::toSpeed(axis_.muGrads, speed_));

    float out = UnitsCalc::toPos(axis_.muGrads, pos_);
    if (axis_.muGrads)
        out = out - ((static_cast<int>(out / 360)) * 360);
    vvr_pos_->set_value(out);

    setStatus(status);
}

