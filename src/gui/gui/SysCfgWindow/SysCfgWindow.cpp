#include "SysCfgWindow.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QPushButton>

#include <SysCfgWindow/AxisSettingsWidget.h>
#include <SysCfgWindow/ModbusUnitCtrlWidget/ModbusUnitCtrlWidget.h>
#include <SysCfgWindow/HardwareSetsWidget.h>
#include <SysCfgWindow/FcCfgWidget/LimitsWidget.h>
#include <SysCfgWindow/LockCtrlWindow/LockCtrlWindow.h>

#include "SysSetsWidget.h"
#include "BkiCfgWidget.h"

SysCfgWindow::SysCfgWindow(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QFont f(font());
    f.setPointSize(16);

    tab_ = new QTabWidget(this);
    tab_->setFont(f);
    tab_->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(tab_);

    axisSettingsW_ = new AxisSettingsWidget(this);
    // modbusCtrlW_ = new ModbusUnitCtrlWidget(this, global::rpc(), global::signer());
    sysSetsW_ = new SysSetsWidget(this);
    hwSetsW_ = new HardwareSetsWidget(this);
    fcLimitsW_ = new LimitsWidget(this, "fc");
    bkiCfgW_ = new BkiCfgWidget(this);
    lock_ctl_ = new LockCtrlWindow(this);

    tab_->addTab(sysSetsW_, "Система");
    tab_->addTab(axisSettingsW_, "Настройки ШД");
    tab_->addTab(hwSetsW_, "Hardware");
    // tab_->addTab(modbusCtrlW_, "Регистры");
    tab_->addTab(fcLimitsW_, "Тоководы");
    tab_->addTab(lock_ctl_, "Блокировки");
    tab_->addTab(bkiCfgW_, "БКИ");
}

void SysCfgWindow::set_guid(int guid)
{
    axisSettingsW_->set_guid(guid);
    sysSetsW_->set_guid(guid);
    // fcLimitsW_->set_guid(guid);

    tab_->setTabVisible(tab_->indexOf(axisSettingsW_), false);
    tab_->setTabVisible(tab_->indexOf(sysSetsW_), false);
    tab_->setTabVisible(tab_->indexOf(fcLimitsW_), false);
    tab_->setTabVisible(tab_->indexOf(hwSetsW_), false);
    // tab_->setTabVisible(tab_->indexOf(modbusCtrlW_), false);
    tab_->setTabVisible(tab_->indexOf(bkiCfgW_), false);

    // switch(guid)
    // {
    // case auth_manufacturer:
    //     tab_->setTabVisible(tab_->indexOf(hwSetsW_), true);
    //     tab_->setTabVisible(tab_->indexOf(modbusCtrlW_), true);
    //     tab_->setTabVisible(tab_->indexOf(bkiCfgW_), true);
    // case auth_engineer:
    //     tab_->setTabVisible(tab_->indexOf(axisSettingsW_), true);
    //     tab_->setTabVisible(tab_->indexOf(sysSetsW_), true);
    //     tab_->setTabVisible(tab_->indexOf(fcLimitsW_), true);
    // case auth_user:
    //     break;
    // }
}

bool SysCfgWindow::have_tabs() const
{
    std::size_t visible = 0;
    for (int i = 0; i < tab_->count(); ++i)
        visible += tab_->isTabVisible(i) ? 1 : 0;
    return visible != 0;
}


