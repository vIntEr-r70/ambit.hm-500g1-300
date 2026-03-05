#pragma once

#include <QWidget>

class QLabel;

class pair_buttons final
    : public QWidget
{
    Q_OBJECT

    QLabel *led_{ nullptr };

public:

    pair_buttons(QWidget *, QString, bool = true);

public:

    void led(bool);

signals:

    void turn_on_pressed();

    void turn_on_released();

    void turn_off_pressed();

    void turn_off_released();
};
