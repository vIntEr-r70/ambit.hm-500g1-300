#pragma once

#include <QWidget>

class QTabWidget;
class AxisSettingsWidget;
class ModbusUnitCtrlWidget;
class SysSetsWidget;
class HardwareSetsWidget;
class LimitsWidget;
class LockCtrlWindow;
class BkiCfgWidget;

class SysCfgWindow
    : public QWidget
{
public:

    SysCfgWindow(QWidget*);

public:

    void set_guid(int);

    bool have_tabs() const;

private:

    QTabWidget* tab_;

    AxisSettingsWidget* axisSettingsW_;
    ModbusUnitCtrlWidget* modbusCtrlW_;
    SysSetsWidget* sysSetsW_;
    HardwareSetsWidget* hwSetsW_;
    LimitsWidget *fcLimitsW_;
    BkiCfgWidget *bkiCfgW_;
    LockCtrlWindow *lock_ctl_;
};

