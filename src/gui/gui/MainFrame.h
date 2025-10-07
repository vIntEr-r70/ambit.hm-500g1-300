#pragma once

#include <QWidget>

#include "func-map.h"
#include <axis-cfg.h>

#include <aem/types.h>

class BkiLockMessage;
class EmgStopMessage;
class NavigationPanel;
class SysCfgWindow;
class AutoCtrlWindow; 

class MainFrame
    : public QWidget
{
    Q_OBJECT

public:

    MainFrame();

    ~MainFrame();

public:

    void nf_sys_mode(aem::uint8) noexcept;

    void nf_sys_ctrl(aem::uint8) noexcept;

    void nf_sys_bki_lock(bool) noexcept;
    
    void nf_sys_emg_stop(bool) noexcept;

    void nf_sys_locker(aem::uint8) noexcept;

private slots:

    void on_login(int) noexcept;

private:

    void on_logout() noexcept;
    
    void keyPressEvent(QKeyEvent*) noexcept override final;

private:

    NavigationPanel *NavigationPanel_;
    SysCfgWindow *sys_cfg_window_;
    AutoCtrlWindow *auto_ctrl_window_;

    func_multi_map sys_key_map_;
    we::axis_cfg axis_cfg_;

    BkiLockMessage *bki_lock_msg_;
    bool bki_lock_flag_{ false };
    bool allow_bki_lock_msg_{ false };
    
    EmgStopMessage *emg_stop_msg_;
    bool emg_stop_flag_{ false };
};

