#include "SprayerCtrlWidget.h"

#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>

#include <Widgets/ValueViewReal.h>
#include <Widgets/ValueSetBool.h>

SprayerCtrlWidget::SprayerCtrlWidget(QWidget* parent, std::string_view name, QString const& title, bool fc, bool dp)
    : QWidget(parent)
    , eng::sibus::node(name)
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
        QHBoxLayout *hL = new QHBoxLayout();
        {
            QLabel *lbl = new QLabel(QString("Спрейер %1").arg(title), this);
            lbl->setStyleSheet("color: gray; font: 14pt");
            hL->addWidget(lbl);

            lbl_block_ = new QLabel(this);
            lbl_block_->setFixedSize({ 24, 24 });
            lbl_block_->setScaledContents(true);
            lbl_block_->setPixmap(QPixmap(":/fc.block"));
            hL->addWidget(lbl_block_);
        }
        vL->addLayout(hL);

        vL->addSpacing(20);

        hL = new QHBoxLayout();
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

    ctl_ = node::add_output_wire();
    node::set_wire_status_handler(ctl_, [this]
    {
        update_widget_view();
    });

    if (vvr_dp_)
    {
        node::add_input_port_unsafe("DP", [this](eng::abc::pack args)
        {
            vvr_dp_->setEnabled(args.size() != 0);
            if (args) vvr_dp_->set_value(eng::abc::get<double>(args));
        });
    }

    if (vvr_fc_)
    {
        node::add_input_port_unsafe("FC", [this](eng::abc::pack args)
        {
            vvr_fc_->setEnabled(args.size() != 0);
            if (args) vvr_fc_->set_value(eng::abc::get<double>(args));
        });
    }

    if (vvr_dp_)
        vvr_dp_->setEnabled(false);

    if (vvr_fc_)
        vvr_fc_->setEnabled(false);

    update_widget_view();
}

void SprayerCtrlWidget::change_state() noexcept
{
    if (vsb_->value())
        node::activate(ctl_, { });
    else
        node::deactivate(ctl_);
}

void SprayerCtrlWidget::update_widget_view()
{
    vsb_->setReadOnly(node::is_blocked(ctl_));

    lbl_block_->setVisible(node::is_blocked(ctl_));

    if (node::is_active(ctl_))
        vsb_->set_value(true);
    else if (node::is_ready(ctl_))
        vsb_->set_value(false);
}

