#include "titled-led.hpp"

#include <QLabel>
#include <QVBoxLayout>

titled_led::titled_led(QWidget *parent, QColor color, QString title)
    : QWidget(parent)
    , color_(color)
{
    QFont f(font());
    f.setPointSize(11);
    setFont(f);

    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        QLabel *lbl = new QLabel(this);
        lbl->setText(title);
        lbl->setAlignment(Qt::AlignCenter);
        vL->addWidget(lbl);

        led_ = new QLabel(this);
        led_->setFrameStyle(QFrame::Box);
        led_->setFixedHeight(40);
        led_->setFixedWidth(40);
        vL->addWidget(led_);
        vL->setAlignment(led_, Qt::AlignHCenter);
    }
}
