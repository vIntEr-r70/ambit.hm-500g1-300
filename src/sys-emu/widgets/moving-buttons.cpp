#include "moving-buttons.hpp"

#include <Widgets/RoundButton.h>
#include <defines.hpp>

#include <QHBoxLayout>
#include <QLabel>

moving_buttons::moving_buttons(QWidget *parent, QString title)
    : QWidget(parent)
{
    setFixedWidth(150);

    QFont f(font());
    f.setPointSize(16);
    setFont(f);

    QVBoxLayout *vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    {
        QLabel *lbl = new QLabel(this);
        lbl->setText(title);
        lbl->setAlignment(Qt::AlignCenter);
        vL->addWidget(lbl);

        QHBoxLayout *hL = new QHBoxLayout();
        {
            RoundButton *btn = new RoundButton(this);
            btn->setIcon(":/cnc.move-dec-x");
            btn->setBgColor(iw::color::green);
            connect(btn, &RoundButton::clicked, [this] { turn_backward(); });
            hL->addWidget(btn);

            btn = new RoundButton(this);
            btn->setIcon(":/cnc.move-inc-x");
            btn->setBgColor(iw::color::green);
            connect(btn, &RoundButton::clicked, [this] { turn_forward(); });
            hL->addWidget(btn);
        }
        vL->addLayout(hL);
    }
}

void moving_buttons::turn_forward()
{
}

void moving_buttons::turn_backward()
{
}
