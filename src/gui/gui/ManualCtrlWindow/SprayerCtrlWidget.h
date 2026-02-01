#pragma once

#include <QWidget>

// #include "common/Defs.h"
// #include <aem/json.hpp>

class ValueViewReal;
class ValueSetBool;

class SprayerCtrlWidget
    : public QWidget
{
    Q_OBJECT

public:

    SprayerCtrlWidget(QWidget*, std::string const&, QString const&, bool, bool);

public:

    void nf_sys_mode(unsigned char) noexcept;

private:

    void change_state() noexcept;

    void updateGui();

private:

    std::string sid_;

    ValueSetBool *vsb_;
    ValueViewReal *vvr_fc_{ nullptr };
    ValueViewReal *vvr_dp_{ nullptr };

    // unsigned char mode_ = Core::SysMode::No;
};

