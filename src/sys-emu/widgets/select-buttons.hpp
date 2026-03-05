#pragma once

#include <QWidget>

class RoundButton;

class select_buttons final
    : public QWidget
{
    Q_OBJECT

    QString key_;
    std::vector<RoundButton*> buttons_;

    RoundButton *selected_{ nullptr };

public:

    select_buttons(QWidget *, QString, QString, std::initializer_list<std::string_view>);

signals:

    void selected(QString const &);

private:

    void switch_selection(RoundButton *);

    void load_state(QString const &);

    void save_state();
};
