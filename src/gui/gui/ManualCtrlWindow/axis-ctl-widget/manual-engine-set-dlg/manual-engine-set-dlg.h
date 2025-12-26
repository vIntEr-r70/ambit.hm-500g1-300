#pragma once

#include <InteractWidgets/InteractWidget.h>
#include <eng/abc/pack.hpp>

class QLabel;
class QLineEdit;
class RoundButton;
class QStackedWidget;
class NumberCalcField;
class axis_ctl_widget;

class manual_engine_set_dlg
    : public InteractWidget 
{
    Q_OBJECT

    axis_ctl_widget *axis_ctl_;

public:

    manual_engine_set_dlg(QWidget *);

public:

    void set_axis(char, std::string_view);

    void set_axis_name(QString const &);

public:

    void set_calibrate_axis(char) noexcept;

    void set_calibrate_step(int) noexcept;

    void set_position(char, float) noexcept;

    void prepare(char) noexcept;

private slots:

    void onKeyPressed(int);

    void onEnterBtn();

    void onCancelBtn();

    void onACKey();

    void onBackspaceKey();

signals:

    void axis_command(eng::abc::pack);

    void grab_axis(char);

    void apply_axis(char);

    void apply_pos(double);

    void doCalibrate(char);

    void doZero(char);

private:

    void showEvent(QShowEvent*) override final;

    // void timerEvent(QTimerEvent*) override final { on_calibrate_timer(); }

private:

    // void to_calibration_window() noexcept;

    // void on_calibrate_timer() noexcept;

    // void update_calibrate_status() noexcept;

    float pos() const noexcept;

private:

    QWidget* create_calibration_page() noexcept;

private:

    // we::axis_cfg const &axis_cfg_;
    std::unordered_map<char, float> axis_pos_;

    QStackedWidget *stack_;

    QLabel *title_[2];

    char calibrateId_{ 0 };
    char axisId_{ 0 };

    // Стриница движения
    NumberCalcField *move_to_pos_;
    QLineEdit* le_;

    std::map<int, RoundButton*> buttons_;
    RoundButton *apply_new_pos_;
    RoundButton *btn_to_zero_;
    RoundButton *btn_to_calibrate_;

    // Страница калибровки
    RoundButton *start_calibration_;

    QLabel *lbl_calibrate_status_;
    std::array<QLabel*, 4> lbl_calibrate_info_;

    std::array<QLabel*, 4> lbl_calibrate_steps_;

    int calibrate_timer_id_{ -1 };
    std::size_t calibrate_step_{ 0 };
    bool calibrate_blink_;
    bool calibrate_error_;
};


