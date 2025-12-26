#pragma once

#include <QWidget>

#include <aem/types.h>

class QLabel;
class ValueViewReal;
class QFrame;

class AxisCtlItemWidget
    : public QWidget
{
    Q_OBJECT

public:

    AxisCtlItemWidget(QWidget*);

signals:

    void on_move_to_click();

public:

    void update_name(std::string_view);

    void update_current_position(double);

    void update_current_speed(double);

    QString const &name() const noexcept { return name_; }

    void set_active(bool);

private:

    void update_gui();

public:

    void apply_self_axis() noexcept;

    void nf_speed(float) noexcept;

    void nf_ls_min(bool v) noexcept { ls_min_max(0, v); }

    void nf_ls_max(bool v) noexcept { ls_min_max(1, v); }

    void nf_pos(float) noexcept;

public:

    void set_sys_ctrl_mode_axis(char) noexcept;

    void set_sys_mode(unsigned char) noexcept;

    void set_sys_ctrl(unsigned char) noexcept;

    void set_sys_error(unsigned int) noexcept;

private:

    void mousePressEvent(QMouseEvent*) override final;

private:

    void ls_min_max(std::size_t, bool) noexcept;

    void updateGui();

private:

    QString name_;

    QLabel* lblHeader_;
    QFrame* lblLS_[2];

    ValueViewReal *vvr_speed_;
    ValueViewReal *vvr_pos_;

    bool active_{ false };

    float pos_{0.0f};
    float speed_{0.0f};

    aem::uint8 sysMode_;
    aem::uint8 ctlMode_;
    bool ls_[2] = {false, false};

    char ctrlModeAxis_ = 0;
    aem::uint32 error_ = 0;
};

