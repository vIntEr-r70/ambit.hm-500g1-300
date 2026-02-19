#pragma once

#include <QWidget>

#include <eng/sibus/node.hpp>

class LockMessageBox;
class NavigationPanel;
class SysCfgWindow;
class auto_ctl_window;

class MainFrame
    : public QWidget
    , public eng::sibus::node
{
    Q_OBJECT

public:

    MainFrame();

    ~MainFrame();

private slots:

    void on_login(int) noexcept;

private:

    void nf_sys_mode(std::string_view) noexcept;

    void on_logout() noexcept;

    void keyPressEvent(QKeyEvent*) noexcept override final;

private:

    void register_on_bus_done() override final;

private:

    NavigationPanel *NavigationPanel_;
    SysCfgWindow *sys_cfg_window_;
    auto_ctl_window *auto_ctl_window_;

    LockMessageBox *lock_msg_box_;
};

