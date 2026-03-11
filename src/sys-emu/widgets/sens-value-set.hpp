#pragma once

#include <QWidget>

class QLabel;
class QScrollBar;

class sens_value_set final
    : public QWidget
{
    Q_OBJECT

    QLabel *value_;
    QScrollBar *sbv_;
    QString key_;

    double k_{ 1.0 };

public:

    sens_value_set(QWidget *, QString, double = 10.0);

signals:

    void change_value(int);

private:

    void update_value(int);

    void load_state(int);

    void save_state();
};
