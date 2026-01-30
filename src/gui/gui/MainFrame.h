#pragma once

#include <QWidget>

class BkiLockMessage;
class EmgStopMessage;
class NavigationPanel;
class SysCfgWindow;
class auto_mode_window;

class MainFrame
    : public QWidget
{
    Q_OBJECT

public:

    MainFrame();

    ~MainFrame();

public:

    void nf_sys_mode(std::uint8_t) noexcept;

    void nf_sys_ctrl(std::uint8_t) noexcept;

    void nf_sys_bki_lock(bool) noexcept;

    void nf_sys_emg_stop(bool) noexcept;

    void nf_sys_locker(std::uint8_t) noexcept;

private slots:

    void on_login(int) noexcept;

private:

    void on_connected() noexcept;

    void on_logout() noexcept;

    void keyPressEvent(QKeyEvent*) noexcept override final;

private:

    NavigationPanel *NavigationPanel_;
    SysCfgWindow *sys_cfg_window_;
    auto_mode_window *auto_mode_window_;

    BkiLockMessage *bki_lock_msg_;
    bool bki_lock_flag_{ false };
    bool allow_bki_lock_msg_{ false };

    EmgStopMessage *emg_stop_msg_;
    bool emg_stop_flag_{ false };
};

