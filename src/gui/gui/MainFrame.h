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

    void update_mode();

    void on_logout() noexcept;

    void keyPressEvent(QKeyEvent*) noexcept override final;

private:

    void register_on_bus_done() override final;

private:

    NavigationPanel *NavigationPanel_;
    SysCfgWindow *sys_cfg_window_;
    auto_ctl_window *auto_ctl_window_;

    LockMessageBox *lock_msg_box_;

    std::string rcu_mode_;
    std::string auto_mode_;
};

