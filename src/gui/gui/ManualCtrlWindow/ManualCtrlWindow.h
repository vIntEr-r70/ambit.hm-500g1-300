#pragma once

#include <QWidget>

class FcCtrlWidget;
class CenteringCfgWidget;
class SprayerCtrlWidget;

class ManualCtrlWindow
    : public QWidget 
{
    Q_OBJECT

public:

    ManualCtrlWindow(QWidget*) noexcept;

public:

    void nf_sys_mode(unsigned char) noexcept;

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

    FcCtrlWidget* fcCtrlW_;
    CenteringCfgWidget *cCfgW_;
    std::array<SprayerCtrlWidget*, 3> spCtrlW_;
};

