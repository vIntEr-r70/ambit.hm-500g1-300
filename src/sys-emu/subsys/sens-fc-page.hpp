#pragma once

#include <QWidget>

class sens_fc_page final
    : public QWidget
{
public:

    sens_fc_page(QWidget *);

private:

    void update_register_value(std::size_t, std::int16_t);
};

