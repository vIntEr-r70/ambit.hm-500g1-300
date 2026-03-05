#pragma once

#include <QWidget>

class sens_dp_page final
    : public QWidget
{
public:

    sens_dp_page(QWidget *);

private:

    void update_register_value(std::size_t, std::int16_t);
};

