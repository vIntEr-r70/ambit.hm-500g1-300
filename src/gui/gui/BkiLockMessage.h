#pragma once

#include "InteractWidgets/MessageBox.h"

class BkiLockMessage final
    : public MessageBox
{
public:

    BkiLockMessage(QWidget*);

public:

    void show();

private:

    void on_button_click();
};
