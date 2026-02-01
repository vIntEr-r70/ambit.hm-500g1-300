#pragma once

#include <QWidget>

class ValueSetBool;
class ValueSetReal;
class RoundButton;
class CenteringCtrlDlg;
class QStackedWidget;

class CenteringCfgWidget final
    : public QWidget
{
public:

    CenteringCfgWidget(QWidget*) noexcept;

public:

    void nf_sys_mode(unsigned char) noexcept;

    void set_centering_step(int) noexcept;

private:

    void unit_type_changed() noexcept;

    void show_dialog() noexcept;

    void updateGui();

private:

    RoundButton *btn_init_;

    ValueSetBool *vsb_type_;
    ValueSetBool *vsb_tooth_type_;
    ValueSetReal *vsr_tooth_shift_;

    CenteringCtrlDlg *centeringCtrlDlg_;

    QStackedWidget *stack_;

    // unsigned char mode_ = Core::SysMode::No;
};
