#pragma once

#include <QWidget>

class QLabel;

class titled_led final
    : public QWidget
{
    QLabel *led_;
    QColor color_;

public:

    titled_led(QWidget *, QColor, QString);
};
