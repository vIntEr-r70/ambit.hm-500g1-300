#pragma once

#include <QWidget>

#include <eng/sibus/node.hpp>

class ValueViewReal;
class ValueSetBool;
class QLabel;

class SprayerCtrlWidget
    : public QWidget
    , public eng::sibus::node
{
    eng::sibus::output_wire_id_t ctl_;

public:

    SprayerCtrlWidget(QWidget*, std::string_view, QString const&, bool, bool);

private:

    void change_state() noexcept;

    void update_widget_view();

private:

    ValueSetBool *vsb_;
    ValueViewReal *vvr_fc_{ nullptr };
    ValueViewReal *vvr_dp_{ nullptr };

    QLabel *lbl_block_;
};

