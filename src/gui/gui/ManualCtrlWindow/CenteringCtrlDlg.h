#pragma once

#include <InteractWidgets/InteractWidget.h>

class QLabel;

class CenteringCtrlDlg 
    : public InteractWidget 
{
public:

    CenteringCtrlDlg(QWidget*) noexcept;

public:

    void show(float, bool) noexcept;

    void show() noexcept;
 
    void set_centering_step(int) noexcept;

private:

    void start_timer() noexcept;

    void on_step_timer() noexcept;

    void on_start() noexcept;

    void on_terminate() noexcept;

private:

    void timerEvent(QTimerEvent*) override final { on_step_timer(); }

private:

    QLabel *lbl_title_;

    std::vector<QLabel*> lbls_; 
    std::vector<QLabel*> lbl_steps_; 

    bool step_error_{ false };
    std::size_t step_{ 0 };
    bool step_blink_{ false };

    int timer_id_{ -1 };

    bool type_;
    float tooth_shift_;
    bool tooth_type_;
};
