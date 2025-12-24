#pragma once

#include <QWidget>

#include <aem/net/rpc_tcp_client.h>

class QVBoxLayout;
class QHBoxLayout;

class FcCtrlWidget;
class CenteringCfgWidget;
class SprayerCtrlWidget;

namespace we {
    class axis_cfg;
}

class ManualCtrlWindow
    : public QWidget 
{
    Q_OBJECT

public:

    ManualCtrlWindow(QWidget*, we::axis_cfg const &) noexcept;

public:

    // void apply_axis_cfg() noexcept;

    // void nf_sys_error(unsigned int) noexcept;

    void nf_sys_mode(unsigned char) noexcept;

    // void nf_sys_ctrl(unsigned char) noexcept;

    // void nf_sys_ctrl_mode_axis(char) noexcept;

    void nf_sys_calibrate(char) noexcept;

    void nf_sys_calibrate_step(int) noexcept;

    void nf_sys_centering_step(int) noexcept;

private:

    void addAxis(char) noexcept;

private slots:

    void engineMakeAsZero(char);

    void engineApplyNewPos(char, float);

    void onDoCalibrate(char) noexcept;

private:

    aem::net::rpc_tcp_client &rpc_;

    we::axis_cfg const &axis_cfg_;

    FcCtrlWidget* fcCtrlW_;
    CenteringCfgWidget *cCfgW_;
    std::array<SprayerCtrlWidget*, 3> spCtrlW_;
};

