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

    QString const &name() const noexcept { return name_; }

private:

    void update_axis_view();

    void update_status(std::uint8_t);

private:

    void mousePressEvent(QMouseEvent*) override final;

private:

    eng::sibus::output_wire_id_t ctl_;

    char axis_;
    QString name_;

    QLabel* lblHeader_;
    QFrame* lblLS_[2];

    ValueViewReal *vvr_speed_;
    ValueViewReal *vvr_pos_;

    bool rotation_;

    bool fault_{ false };
};

