#include "MainFrame.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QMoveEvent>
#include <QFileInfo>
#include <QPushButton>
#include <QKeyEvent>
#include <QStackedWidget>

#include <WelcomeWindow/WelcomeWindow.h>
#include "ManualCtrlWindow/ManualCtrlWindow.h"
#include "AutoCtrlWindow/AutoCtrlWindow.h"
#include "SysCfgWindow/SysCfgWindow.h"
#include "ArcsWindow/ArcsWindow.h"
#include <NotifyWindow/NotifyWindow.h>
#include "MnemonicWindow/MnemonicWindow.h"
#include "LiquidSystem/LiquidSystemWindow.h"

#include "BkiLockMessage.h"
#include "EmgStopMessage.h"
#include <NavigationPanel.h>
#include "Interact.h"

#include <we/locker.h>
#include "common/Defs.h"

#include <global.h>

#include <aem/log.h>

MainFrame::MainFrame()
    : QWidget()
{
    // Создаем верхний слой на данном окне
    Interact::create(this);

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

    // ----------------------------------------------------------------

    // Ждем пока будет установлено соединение с агентом
    global::rpc().on_connected = [this] {
        on_connected();
    };

#ifndef BUILDROOT
    on_connected();
    NavigationPanel_->switch_to("manual");
#endif

    global::subscribe("sys.{}", [this](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    {
        std::string_view kparam = keys[0].get<std::string_view>();
        sys_key_map_.apply(kparam, value);
    });

    emg_stop_msg_ = new EmgStopMessage(this);
    bki_lock_msg_ = new BkiLockMessage(this);
}

MainFrame::~MainFrame()
{
    Interact::destroy();
}

void MainFrame::on_connected() noexcept
{
    ManualCtrlWindow* mcw = new ManualCtrlWindow(this, axis_cfg_);
    NavigationPanel_->add(mcw, "manual",   true);

    LiquidSystemWindow* lsw = new LiquidSystemWindow(this);
    NavigationPanel_->add(lsw, "lsw",   true);

    auto_ctrl_window_ = new AutoCtrlWindow(this, axis_cfg_);
    NavigationPanel_->add(auto_ctrl_window_, "auto", true);

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

    sys_key_map_.add("mode",            mcw, &ManualCtrlWindow::nf_sys_mode);
    sys_key_map_.add("ctrl",            mcw, &ManualCtrlWindow::nf_sys_ctrl);
    sys_key_map_.add("error",           mcw, &ManualCtrlWindow::nf_sys_error);
    sys_key_map_.add("ctrl-mode-axis",  mcw, &ManualCtrlWindow::nf_sys_ctrl_mode_axis);
    sys_key_map_.add("calibrate",       mcw, &ManualCtrlWindow::nf_sys_calibrate);
    sys_key_map_.add("calibrate-step",  mcw, &ManualCtrlWindow::nf_sys_calibrate_step);
    sys_key_map_.add("centering-step",  mcw, &ManualCtrlWindow::nf_sys_centering_step);

    // sys_key_map_.add("error",           dw, &DiagnosticWindow::nf_sys_error);

    sys_key_map_.add("mode",            this, &MainFrame::nf_sys_mode);
    sys_key_map_.add("ctrl",            this, &MainFrame::nf_sys_ctrl);
    sys_key_map_.add("bki-lock",        this, &MainFrame::nf_sys_bki_lock);
    sys_key_map_.add("emg-stop",        this, &MainFrame::nf_sys_emg_stop);
    sys_key_map_.add("locker",          this, &MainFrame::nf_sys_locker);

    sys_key_map_.add("mode",            auto_ctrl_window_, &AutoCtrlWindow::nf_sys_mode);


    // Получаем исходный список конфигурации осей
    global::rpc().call("get", { "cnc", "axis-cfg", { } })
        .done([this, mcw](nlohmann::json const& ret)
        {
            for (auto& [key, val] : ret.items())
            {
                char axisId = key.at(0);
                axis_cfg_.base64(axisId, val.get<std::string_view>());
            }
            mcw->apply_axis_cfg();
            auto_ctrl_window_->apply_axis_cfg();
        });
}


void MainFrame::on_login(int guid) noexcept
{
    global::rpc().call("set", { "sys", "login", { true } })
        .done([this, guid](nlohmann::json const&)
        {
            allow_bki_lock_msg_ = (guid != auth_manufacturer);

            if (bki_lock_flag_ && allow_bki_lock_msg_)
                bki_lock_msg_->show();

            sys_cfg_window_->set_guid(guid);
            auto_ctrl_window_->set_guid(guid);
            NavigationPanel_->hide("scfg", !sys_cfg_window_->have_tabs());

            NavigationPanel_->switch_to("manual");
        });
}

void MainFrame::on_logout() noexcept
{
    allow_bki_lock_msg_ = false;
    global::rpc().call("set", { "sys", "login", { false } });
}

void MainFrame::nf_sys_mode(aem::uint8 mode) noexcept
{
    switch(mode)
    {
    case Core::SysMode::No:
        NavigationPanel_->set_top_label("- - -", Qt::white);
        break;
    case Core::SysMode::Manual:
        NavigationPanel_->set_top_label("Ручной", QColor(0xFFB800));
        break;
    case Core::SysMode::Auto:
        NavigationPanel_->set_top_label("Авто", QColor(0xFFB800));
        break;
    case Core::SysMode::Error:
        NavigationPanel_->set_top_label("Ошибка", QColor(0xF83232));
        break;
    }
}

void MainFrame::nf_sys_ctrl(aem::uint8 ctrl) noexcept
{
    switch(ctrl)
    {
    case Core::CtlMode::No:
        NavigationPanel_->set_bottom_label("- - -", Qt::white);
        break;
    case Core::CtlMode::Panel:
        NavigationPanel_->set_bottom_label("Панель", QColor(0xFFB800));
        break;
    case Core::CtlMode::Rcu:
        NavigationPanel_->set_bottom_label("Пульт", QColor(0xFFB800));
        break;
    }
}

void MainFrame::nf_sys_bki_lock(bool lock) noexcept
{
    bki_lock_flag_ = lock;

    if (lock && !allow_bki_lock_msg_)
        return;

    if (lock)
        bki_lock_msg_->show();
    else
        bki_lock_msg_->hide();
}

void MainFrame::nf_sys_emg_stop(bool lock) noexcept
{
    emg_stop_flag_ = lock;

    if (lock)
        emg_stop_msg_->show();
    else
        emg_stop_msg_->hide();
}

void MainFrame::nf_sys_locker(aem::uint8 status) noexcept
{
    switch(static_cast<we::locker::item_status>(status))
    {
    case we::locker::item_status::normal:
        NavigationPanel_->set_notify_state("mimic", 0);
        break;
    case we::locker::item_status::warning:
        NavigationPanel_->set_notify_state("mimic", 'w');
        break;
    case we::locker::item_status::critical:
        NavigationPanel_->set_notify_state("mimic", 'e');
        break;
    }
}

void MainFrame::keyPressEvent(QKeyEvent *e) noexcept
{
    if (e->key() == Qt::Key_Escape)
        hide();
}
