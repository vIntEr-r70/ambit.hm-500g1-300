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

    void update_axis_state();

public:

    // void nf_ls_min(bool v) noexcept { ls_min_max(0, v); }
    //
    // void nf_ls_max(bool v) noexcept { ls_min_max(1, v); }

private:

    void mousePressEvent(QMouseEvent*) override final;

    // void ls_min_max(std::size_t, bool) noexcept;

private:

    eng::sibus::output_wire_id_t ctl_;

    char axis_;
    QString name_;

    QLabel* lblHeader_;
    QFrame* lblLS_[2];

    ValueViewReal *vvr_speed_;
    ValueViewReal *vvr_pos_;

    bool rotation_;

    // float pos_{0.0f};
    // float speed_{0.0f};

    // aem::uint8 sysMode_;
    // aem::uint8 ctlMode_;
    // bool ls_[2] = {false, false};

    // char ctrlModeAxis_ = 0;
    // aem::uint32 error_ = 0;
};

