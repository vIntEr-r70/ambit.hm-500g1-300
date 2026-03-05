#include "pair-buttons.hpp"

#include <Widgets/RoundButton.h>
#include <defines.hpp>

#include <QHBoxLayout>
#include <QLabel>

pair_buttons::pair_buttons(QWidget *parent, QString title, bool led)
    : QWidget(parent)
{
    setFixedWidth(150);

    QFont f(font());
    f.setPointSize(11);
    setFont(f);

    QVBoxLayout *vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    {
        QLabel *lbl = new QLabel(this);
        lbl->setText(title);
        lbl->setAlignment(Qt::AlignCenter);
        vL->addWidget(lbl);

        QVBoxLayout *vvL = new QVBoxLayout();
        {
            RoundButton *btn = new RoundButton(this);
            btn->setText("ON");
            btn->setBgColor(iw::color::green);
            btn->setTextColor(Qt::white);
            connect(btn, &RoundButton::pressed, [this] { emit turn_on_pressed(); });
            connect(btn, &RoundButton::released, [this] { emit turn_on_released(); });
            vvL->addWidget(btn);

            if (led)
            {
                led_ = new QLabel(this);
                led_->setFrameStyle(QFrame::Box);
                led_->setFixedHeight(20);
                vvL->addWidget(led_);
            }

            btn = new RoundButton(this);
            btn->setText("OFF");
            btn->setBgColor(iw::color::red);
            btn->setTextColor(Qt::white);
            connect(btn, &RoundButton::pressed, [this] { emit turn_off_pressed(); });
            connect(btn, &RoundButton::released, [this] { emit turn_off_released(); });
            vvL->addWidget(btn);
        }
        vL->addLayout(vvL);
    }
}

void pair_buttons::led(bool value)
{
    if (led_ != nullptr)
        led_->setStyleSheet(value ? "background-color: yellow" : "");
}

