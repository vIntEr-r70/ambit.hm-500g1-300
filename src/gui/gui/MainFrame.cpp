#include "MainFrame.h"
#include "ManualCtrlWindow/ManualCtrlWindow.h"
#include "LiquidSystem/LiquidSystemWindow.h"
#include "auto-ctl-window/auto-ctl-window.hpp"
#include "SysCfgWindow/SysCfgWindow.h"
#include "ArcsWindow/ArcsWindow.h"

#include <WelcomeWindow/WelcomeWindow.h>
#include <NotifyWindow/NotifyWindow.h>
#include <NavigationPanel.h>
#include <MnemonicWindow/MnemonicWindow.h>

#include "LockMessageBox.hpp"
#include "Interact.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QMoveEvent>
#include <QFileInfo>
#include <QPushButton>
#include <QKeyEvent>
#include <QStackedWidget>

MainFrame::MainFrame()
    : QWidget()
    , eng::sibus::node("main-gui-frame")
{
    // Создаем верхний слой на данном окне
    Interact::create(this);

    lock_msg_box_ = new LockMessageBox(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QStackedWidget* sw = new QStackedWidget(this);
    layout->addWidget(sw);

    NavigationPanel_ = new NavigationPanel(this, sw);
    connect(NavigationPanel_, &NavigationPanel::logout, [this] { on_logout(); });
    layout->addWidget(NavigationPanel_);

    // Создаем рабочие окна ------------------------------------------

    QWidget* w = new WelcomeWindow(this);
    connect(w, SIGNAL(login(int)), this, SLOT(on_login(int)));
    NavigationPanel_->add(w, "auth", false);

    node::add_input_port_unsafe("mode", [this](eng::abc::pack args)
    {
        nf_sys_mode(args ? eng::abc::get_sv(args) : "");
    });
}

MainFrame::~MainFrame()
{
    Interact::destroy();
}

void MainFrame::register_on_bus_done()
{
    ManualCtrlWindow* mcw = new ManualCtrlWindow(this);
    NavigationPanel_->add(mcw, "manual",   true);

    LiquidSystemWindow* lsw = new LiquidSystemWindow(this);
    NavigationPanel_->add(lsw, "lsw",   true);

    auto_ctl_window_ = new auto_ctl_window(this);
    NavigationPanel_->add(auto_ctl_window_, "auto", true);

    // NavigationPanel_->add(new ArcsWindow(this), "arcs", true);

    sys_cfg_window_ = new SysCfgWindow(this);
    NavigationPanel_->add(sys_cfg_window_, "scfg", true);

    NavigationPanel_->add(new MnemonicWindow(this), "mimic", true);

    auto nw = new NotifyWindow(this);
    connect(nw, &NotifyWindow::update_state, [this](char state)
        { NavigationPanel_->set_notify_state("diag", state); });
    NavigationPanel_->add(nw, "diag", true);

    // auto dw = new DiagnosticWindow(this);
    // NavigationPanel_->add(dw, "diag", true);

    // // sys_key_map_.add("error",           mcw, &ManualCtrlWindow::nf_sys_error);
    // // sys_key_map_.add("ctrl-mode-axis",  mcw, &ManualCtrlWindow::nf_sys_ctrl_mode_axis);
    // sys_key_map_.add("calibrate",       mcw, &ManualCtrlWindow::nf_sys_calibrate);
    // sys_key_map_.add("calibrate-step",  mcw, &ManualCtrlWindow::nf_sys_calibrate_step);
    // sys_key_map_.add("centering-step",  mcw, &ManualCtrlWindow::nf_sys_centering_step);
    //
    // // sys_key_map_.add("error",           dw, &DiagnosticWindow::nf_sys_error);
    //
    // sys_key_map_.add("mode",            this, &MainFrame::nf_sys_mode);
    // sys_key_map_.add("ctrl",            this, &MainFrame::nf_sys_ctrl);
    // sys_key_map_.add("bki-lock",        this, &MainFrame::nf_sys_bki_lock);
    // sys_key_map_.add("emg-stop",        this, &MainFrame::nf_sys_emg_stop);
    // sys_key_map_.add("locker",          this, &MainFrame::nf_sys_locker);

    // sys_key_map_.add("mode",            auto_ctrl_window_, &AutoCtrlWindow::nf_sys_mode);


    // Получаем исходный список конфигурации осей
    // global::rpc().call("get", { "cnc", "axis-cfg", { } })
    //     .done([this, mcw](nlohmann::json const& ret)
    //     {
    //         for (auto& [key, val] : ret.items())
    //         {
    //             char axisId = key.at(0);
    //             axis_cfg_.base64(axisId, val.get<std::string_view>());
    //         }
    //         mcw->apply_axis_cfg();
    //         auto_ctrl_window_->apply_axis_cfg();
    //     });

#ifdef BUILDROOT
    NavigationPanel_->switch_to("manual");
    lock_msg_box_->allow(true);
#else
    NavigationPanel_->switch_to("auto");
    lock_msg_box_->allow(true);
#endif

    nf_sys_mode("");
}

void MainFrame::on_login(int guid) noexcept
{
    lock_msg_box_->allow(true);

    // global::rpc().call("set", { "sys", "login", { true } })
    //     .done([this, guid](nlohmann::json const&)
    //     {
    //         allow_bki_lock_msg_ = (guid != auth_manufacturer);
    //
    //         if (bki_lock_flag_ && allow_bki_lock_msg_)
    //             bki_lock_msg_->show();
    //
    //         sys_cfg_window_->set_guid(guid);
    //         // auto_ctrl_window_->set_guid(guid);
    //         NavigationPanel_->hide("scfg", !sys_cfg_window_->have_tabs());
    //
    //         NavigationPanel_->switch_to("manual");
    //     });
}

void MainFrame::on_logout() noexcept
{
    lock_msg_box_->allow(false);
}

void MainFrame::nf_sys_mode(std::string_view mode) noexcept
{
    NavigationPanel_->set_top_label("- - -", Qt::white);
    NavigationPanel_->set_bottom_label("- - -", Qt::white);

    if (mode == "manual" || mode == "rcu")
    {
        NavigationPanel_->set_top_label("Ручной", QColor(0xFFB800));
        NavigationPanel_->set_bottom_label((mode == "rcu") ? "Пульт" : "Панель", QColor(0xFFB800));
    }
    else if (mode == "auto")
    {
        NavigationPanel_->set_top_label("Авто", QColor(0xFFB800));
    }
}

// void MainFrame::nf_sys_bki_lock(bool lock) noexcept
// {
//     bki_lock_flag_ = lock;
//
//     if (lock && !allow_bki_lock_msg_)
//         return;
//
//     if (lock)
//         bki_lock_msg_->show();
//     else
//         bki_lock_msg_->hide();
// }
//
// void MainFrame::nf_sys_emg_stop(bool lock) noexcept
// {
//     emg_stop_flag_ = lock;
//
//     if (lock)
//         emg_stop_msg_->show();
//     else
//         emg_stop_msg_->hide();
// }

// void MainFrame::nf_sys_locker(std::uint8_t status) noexcept
// {
//     // switch(static_cast<we::locker::item_status>(status))
//     // {
//     // case we::locker::item_status::normal:
//     //     NavigationPanel_->set_notify_state("mimic", 0);
//     //     break;
//     // case we::locker::item_status::warning:
//     //     NavigationPanel_->set_notify_state("mimic", 'w');
//     //     break;
//     // case we::locker::item_status::critical:
//     //     NavigationPanel_->set_notify_state("mimic", 'e');
//     //     break;
//     // }
// }

void MainFrame::keyPressEvent(QKeyEvent *e) noexcept
{
    if (e->key() == Qt::Key_Escape)
        hide();
}


