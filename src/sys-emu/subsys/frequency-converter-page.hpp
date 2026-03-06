#pragma once

#include <QWidget>

#include <bitset>
#include <array>

class RoundButton;
class QLabel;

class frequency_converter_page final
    : public QWidget
{
    struct error_mask_t
    {
        std::array<RoundButton *, 16> buttons;
        std::bitset<16> mask;
    };
    std::array<error_mask_t, 4> errors_;

    std::bitset<2> status_;

    QLabel *lbl_powered_;
    QLabel *lbl_I_set_;
    QLabel *lbl_U_set_;
    QLabel *lbl_P_set_;

public:

    frequency_converter_page(QWidget *);

private:

    void init_callbacks();

    void invert_error_mask_bit(std::size_t, std::size_t);

    void update_power_label();
};
