#include "SprayerCtrlWidget.h"

#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>

#include <Widgets/ValueViewReal.h>
#include <Widgets/ValueSetBool.h>

SprayerCtrlWidget::SprayerCtrlWidget(QWidget* parent, std::string const &sid, QString const& title, bool fc, bool dp)
    : QWidget(parent)
    , sid_(sid)
{
	setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("border-radius: 20px; background-color: white");

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 0);
    effect->setColor(Qt::gray);
    effect->setBlurRadius(20);

    setGraphicsEffect(effect);

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    {
        QLabel *lbl = new QLabel(QString("Спрейер %1").arg(title), this);
        lbl->setStyleSheet("color: gray; font: 14pt");
        vL->addWidget(lbl);

        vL->addSpacing(20);

        QHBoxLayout *hL = new QHBoxLayout();
        {
            vsb_ = new ValueSetBool(this, "");
            connect(vsb_, &ValueSetBool::onValueChanged, [this] { change_state(); });
            vsb_->setValueView("ВЫКЛ", "ВКЛ");
            vsb_->setTitle("Вкл/Выкл");
            hL->addWidget(vsb_);

            if (fc)
            {
                vvr_fc_ = new ValueViewReal(this, "л/мин");
                vvr_fc_->set_precision(1);
                vvr_fc_->set_value(12.9);
                vvr_fc_->setTitle("Проток");
                vvr_fc_->setVisible(fc);
                hL->addWidget(vvr_fc_);
            }
            else
            {
                hL->addStretch();
            }

            if (dp)
            {
                vvr_dp_ = new ValueViewReal(this, "Бар");
                vvr_dp_->set_precision(1);
                vvr_dp_->set_value(12.9);
                vvr_dp_->setTitle("Давление");
                vvr_dp_->setVisible(dp);
                hL->addWidget(vvr_dp_);
            }
            else
            {
                hL->addStretch();
            }
        }
        vL->addLayout(hL);
    }

    // if (dp) 
    // {
    //     global::subscribe(fmt::format("{}.DP", sid), [this](nlohmann::json::array_t const&, nlohmann::json const& value) {
    //         if (vvr_dp_) vvr_dp_->setJsonValue(value);
    //     });
    // }
    //
    // if (fc) 
    // {
    //     global::subscribe(fmt::format("{}.FC", sid), [this](nlohmann::json::array_t const&, nlohmann::json const& value) {
    //         if (vvr_fc_) vvr_fc_->setJsonValue(value);
    //     });
    // }
    //
    // global::subscribe(fmt::format("{}.V", sid), [this](nlohmann::json::array_t const&, nlohmann::json const& value) {
    //     vsb_->setJsonValue(value);
    // });
}

void SprayerCtrlWidget::nf_sys_mode(unsigned char v) noexcept
{
    // if (mode_ == v)
    //     return;
    // mode_ = v;
    // updateGui();
}

void SprayerCtrlWidget::change_state() noexcept
{
    // if (vsb_->value())
    //     global::rpc().call("start", { sid_ });
    // else
    //     global::rpc().call("stop", { sid_ });
}

void SprayerCtrlWidget::updateGui()
{
    // bool allow = (mode_ == Core::SysMode::Manual);
    // vsb_->setEnabled(allow);
}

