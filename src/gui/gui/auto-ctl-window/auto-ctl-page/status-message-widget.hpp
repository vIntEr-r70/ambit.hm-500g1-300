#pragma once

#include <eng/sibus/sibus.hpp>

#include <QWidget>

class QLabel;

class status_message_widget final
    : public QWidget
{
    QLabel *msg_;

    std::unordered_map<eng::sibus::istatus, std::string> msgs_;

public:

    status_message_widget(QWidget *);

public:

    void update(eng::sibus::istatus, std::string_view);

    void reset();
};
