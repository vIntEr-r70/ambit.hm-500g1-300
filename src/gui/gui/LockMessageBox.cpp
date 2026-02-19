#include "LockMessageBox.hpp"

#include <Widgets/RoundButton.h>

#include <QLabel>
#include <QVBoxLayout>

LockMessageBox::LockMessageBox(QWidget *parent)
    : InteractWidget(parent)
    , eng::sibus::node("gui-lock")
{
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout *vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    {
        bki_ = new QWidget(this);
        bki_->setAttribute(Qt::WA_StyledBackground, true);
        bki_->setStyleSheet("QWidget { border-radius: 20px; background-color: white }");
        {
            QVBoxLayout *vL = new QVBoxLayout(bki_);
            vL->setContentsMargins(20, 20, 20, 20);
            {
                QLabel *lbl = new QLabel("Обнаружен контакт с индуктором", bki_);
                lbl->setStyleSheet("font: 16pt; color: #E55056; font-weight: 500;");
                lbl->setWordWrap(true);
                lbl->setAlignment(Qt::AlignCenter);
                vL->addWidget(lbl);

                vL->addSpacing(20);

                QHBoxLayout *hL = new QHBoxLayout();
                hL->setSpacing(15);
                {
                    hL->addStretch();

                    btn_bki_reset_ = new RoundButton(this);
                    btn_bki_reset_->setTextColor("#ffffff");
                    connect(btn_bki_reset_, &RoundButton::clicked, [this]
                    {
                        node::activate(bki_reset_, { bki_action_key_ });
                    });
                    hL->addWidget(btn_bki_reset_);

                    hL->addStretch();
                }
                vL->addLayout(hL);
            }
        }
        vL->addWidget(bki_);

        emg_ = new QWidget(this);
        emg_->setAttribute(Qt::WA_StyledBackground, true);
        emg_->setStyleSheet("QWidget { border-radius: 20px; background-color: white }");
        {
            QVBoxLayout *vL = new QVBoxLayout(emg_);
            vL->setContentsMargins(20, 20, 20, 20);
            {
                QLabel *lbl = new QLabel("Активирован аварийный стоп", emg_);
                lbl->setStyleSheet("font: 16pt; color: #E55056; font-weight: 500;");
                lbl->setWordWrap(true);
                lbl->setAlignment(Qt::AlignCenter);
                vL->addWidget(lbl);
            }
        }
        vL->addWidget(emg_);
    }

    bki_reset_ = node::add_output_wire();

    node::add_input_port("bki", [this](eng::abc::pack args)
    {
        bki_action_key_ = eng::abc::get<std::uint8_t>(args);

        switch (bki_action_key_)
        {
        case 0:
            bki_->setVisible(false);
            break;

        case 1:
            btn_bki_reset_->setText("Снять защиту");
            btn_bki_reset_->setBgColor("#E55056");
            btn_bki_reset_->setEnabled(true);
            bki_->setVisible(true);
            break;

        case 2:
            btn_bki_reset_->setText("Активировать защиту");
            btn_bki_reset_->setBgColor("#29AC39");
            btn_bki_reset_->setEnabled(true);
            bki_->setVisible(true);
            break;

        default:
            bki_action_key_ = 0;
            return;
        }

        update_dlg_visible();
    });

    node::add_input_port("emg", [this](eng::abc::pack args)
    {
        bool active = eng::abc::get<bool>(args);
        emg_->setVisible(active);
        update_dlg_visible();
    });

    bki_->hide();
    emg_->hide();
}

void LockMessageBox::allow(bool allow)
{
    if (allow == allow_)
        return;
    allow_ = allow;

    update_dlg_visible();
}

void LockMessageBox::update_dlg_visible()
{
    if ((bki_->isVisibleTo(this) || emg_->isVisibleTo(this)) && allow_)
    {
        InteractWidget::show();
        return;
    }
    else
    {
        InteractWidget::hide();
    }
}

