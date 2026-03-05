#include "sens-value-set.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>

sens_value_set::sens_value_set(QWidget *parent, QString key)
    : QWidget(parent)
    , key_(key)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        QLabel *lbl = new QLabel(this);
        lbl->setText(key);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setFixedHeight(30);
        vL->addWidget(lbl);

        value_ = new QLabel(this);
        value_->setAlignment(Qt::AlignCenter);
        value_->setFrameStyle(QFrame::Box);
        value_->setFixedHeight(30);
        vL->addWidget(value_);

        sbv_ = new QScrollBar(Qt::Vertical, this);
        sbv_->setStyleSheet("border: 2px solid grey; background: #f0f0f0; width: 30px;");
        sbv_->setRange(-1000, 1000);
        sbv_->setInvertedAppearance(true);
        connect(sbv_, &QScrollBar::valueChanged, [this](int value)
        {
            update_value(value);
            save_state();
        });
        vL->addWidget(sbv_);
        vL->setAlignment(sbv_, Qt::AlignHCenter);
    }

    QTimer::singleShot(0, [this]
    {
        load_state(0);
    });
}

void sens_value_set::update_value(int value)
{
    value_->setText(QString::number(value / 10.0, 'f', 1));
    emit change_value(value);
}

void sens_value_set::load_state(int value)
{
    QSettings settings("sys-emu");
    int v = settings.value(key_, value).toInt();
    sbv_->setValue(v);
    update_value(v);
}

void sens_value_set::save_state()
{
    QSettings settings("sys-emu");
    settings.setValue(key_, sbv_->value());
}
