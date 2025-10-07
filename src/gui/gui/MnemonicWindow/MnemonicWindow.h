#pragma once

#include "MnemonicCommon.h"

#include <QStackedWidget>

class MnemonicWidget;
class LockCtrlWindow;
class ValueSetBool;

class MnemonicWindow final
    : public QStackedWidget
{
    Q_OBJECT

public:

    MnemonicWindow(QWidget*) noexcept;

private:
    
    void switch_mimic_page(std::size_t);
        
private:

    MnemonicWidget *mW_;
    LockCtrlWindow *lcW_;

    MnemonicImages images_;
    
    ValueSetBool *info_;
};
    
