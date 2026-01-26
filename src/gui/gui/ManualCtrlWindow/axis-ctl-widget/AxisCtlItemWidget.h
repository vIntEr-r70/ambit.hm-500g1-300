#pragma once

#include <QWidget>

#include <eng/sibus/node.hpp>

class QLabel;
class ValueViewReal;
class QFrame;

class AxisCtlItemWidget
    : public QWidget
    , public eng::sibus::node
{
    Q_OBJECT

public:

    AxisCtlItemWidget(QWidget*, char, std::string_view, bool);

signals:

    void on_move_to_click();

    void axis_set_speed(double);

    void axis_real_speed(double);

    void axis_position(double);

public:

    void execute(eng::abc::pack);

public:


    QString const &name() const noexcept { return name_; }

private:

    void update_gui();

public:

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

    eng::sibus::output_wire_id_t octl_;

    char axis_;
    QString name_;

    QLabel* lblHeader_;
    QFrame* lblLS_[2];

    ValueViewReal *vvr_speed_;
    ValueViewReal *vvr_pos_;

    bool active_{ false };

    float pos_{0.0f};
    float speed_{0.0f};

    bool rotation_;

    // aem::uint8 sysMode_;
    // aem::uint8 ctlMode_;
    bool ls_[2] = {false, false};

    char ctrlModeAxis_ = 0;
    // aem::uint32 error_ = 0;
};

