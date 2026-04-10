#pragma once

#include <QWidget>

class RoundButton;
class QLabel;

class mimic_page final
    : public QWidget
{
public:

    mimic_page(QWidget *);

private:

    void update_btn_view(RoundButton *, bool);

    void init_H(std::string const &, RoundButton *);

    void init_P(std::string const &, RoundButton *);

    void init_VA(std::string_view, RoundButton *, bool = false);

    void init_VA(std::string_view, QLabel *);

    void init_WH(std::string_view, RoundButton *);

    void load_state(RoundButton *);
};
