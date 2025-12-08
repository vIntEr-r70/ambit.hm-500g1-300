#include "CenteringCfgWidget.h"

#include <Widgets/ValueSetBool.h>
#include <Widgets/ValueSetReal.h>
#include <Widgets/RoundButton.h>

#include "CenteringCtrlDlg.h"

#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>

#include <aem/log.h>

CenteringCfgWidget::CenteringCfgWidget(QWidget* parent) noexcept
    : QWidget(parent)
{
    setStyleSheet("border-radius: 20px; background-color: white");

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 0);
    effect->setColor(Qt::gray);
    effect->setBlurRadius(20);
    setGraphicsEffect(effect);

    QVBoxLayout *vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    {
        QLabel *lbl = new QLabel(this);
        lbl->setText("Позиционирование индуктора");
        lbl->setAlignment(Qt::AlignTop);
        lbl->setStyleSheet("color: gray; font: 14pt");
        vL->addWidget(lbl);

        vL->addSpacing(20);

        QHBoxLayout *hL = new QHBoxLayout();
        {
            QVBoxLayout *vL = new QVBoxLayout();
            {
                vsb_type_ = new ValueSetBool(this);
                vsb_type_->setValueView("Зуб", "Вал");
                connect(vsb_type_, &ValueSetBool::onValueChanged, [this] { unit_type_changed(); });
                vsb_type_->setTitle("Деталь");
                vL->addWidget(vsb_type_);

                vL->addStretch();

                btn_init_ = new RoundButton(this);
                connect(btn_init_, &RoundButton::clicked, [this] { show_dialog(); });
                btn_init_->setText("ПЕРЕЙТИ");
                btn_init_->setBgColor("#29AC39");
                btn_init_->setTextColor(Qt::white);
                btn_init_->setMinimumWidth(100);
                vL->addWidget(btn_init_);
            }
            hL->addLayout(vL);

            stack_ = new QStackedWidget(this);
            {
                QWidget *w = new QWidget(stack_);
                QVBoxLayout *vL = new QVBoxLayout(w);
                vL->setContentsMargins(0, 0, 0, 0);
                {
                    QHBoxLayout *hL = new QHBoxLayout();
                    {
                        vsr_tooth_shift_ = new ValueSetReal(this, "мм");
                        vsr_tooth_shift_->setTitle("Глубина зуба");
                        vsr_tooth_shift_->set_precision(2);
                        vsr_tooth_shift_->set_value(10);
                        hL->addWidget(vsr_tooth_shift_);

                        vsb_tooth_type_ = new ValueSetBool(this);
                        vsb_tooth_type_->setValueView("Внешний", "Внутренний");
                        vsb_tooth_type_->setTitle("Тип зуба");
                        vsb_tooth_type_->setMinimumWidth(100);
                        hL->addWidget(vsb_tooth_type_);
                    }
                    vL->addLayout(hL);

                    vL->addStretch();

                    hL = new QHBoxLayout();
                    {
                        hL->addStretch();

                        QLabel *lbl = new QLabel(this);
                        lbl->setPixmap(QPixmap(":/cnc.tooth"));
                        hL->addWidget(lbl);

                        hL->addStretch();
                    }
                    vL->addLayout(hL);

                    vL->addStretch();
                }
                stack_->addWidget(w);

                w = new QWidget(stack_);
                vL = new QVBoxLayout(w);
                vL->setContentsMargins(0, 0, 0, 0);
                {
                    QLabel *lbl = new QLabel(this);
                    lbl->setPixmap(QPixmap(":/cnc.shaft"));
                    vL->addWidget(lbl);
                    vL->setAlignment(lbl, Qt::AlignCenter);
                }
                stack_->addWidget(w);
            }
            hL->addWidget(stack_);

            hL->setStretch(0, 0);
            hL->setStretch(1, 1);
        }
        vL->addLayout(hL);
    }

    centeringCtrlDlg_ = new CenteringCtrlDlg(this);
    centeringCtrlDlg_->setVisible(false);
}

void CenteringCfgWidget::nf_sys_mode(unsigned char v) noexcept
{
    if (mode_ == v)
        return;
    mode_ = v;
    updateGui();
}

void CenteringCfgWidget::set_centering_step(int v) noexcept
{
    centeringCtrlDlg_->set_centering_step(v);
}

void CenteringCfgWidget::unit_type_changed() noexcept
{
    stack_->setCurrentIndex(vsb_type_->value() ? 1 : 0);
}

void CenteringCfgWidget::show_dialog() noexcept
{
    if (vsb_type_->value())
    {
        centeringCtrlDlg_->show();
    }
    else
    {
        float shift = vsr_tooth_shift_->value();
        bool stype = vsb_tooth_type_->value();
        centeringCtrlDlg_->show(shift, stype);
    }
}

void CenteringCfgWidget::updateGui()
{
    bool allow = (mode_ == Core::SysMode::Manual);
    btn_init_->setEnabled(allow);
}
