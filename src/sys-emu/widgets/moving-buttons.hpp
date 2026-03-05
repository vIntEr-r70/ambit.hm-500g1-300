#pragma once

#include <QWidget>

class moving_buttons final
    : public QWidget
{
public:

    moving_buttons(QWidget *, QString);

private:

    void turn_forward();

    void turn_backward();
};
