#include "LiquidSystemWindow.h"

#include <HardeningEnvironmentWidget/HardeningEnvironmentWidget.h>
#include <QVBoxLayout>

LiquidSystemWindow::LiquidSystemWindow(QWidget *parent)
    : QWidget(parent)
{ 
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        vL->addStretch();
        
        QHBoxLayout *hL = new QHBoxLayout();
        {
            hL->addStretch();
            
            auto *w1 = new HardeningEnvironmentWidget(this, "barrel-0", "Вода ТВЧ");
            w1->setMinimumWidth(260);
            hL->addWidget(w1);
            auto *w2 = new HardeningEnvironmentWidget(this, "barrel-1", "Закалочная вода");
            w2->setMinimumWidth(260);
            hL->addWidget(w2);
            auto *w3 = new HardeningEnvironmentWidget(this, "barrel-2", "Закалочная жидкость");
            w3->setMinimumWidth(260);
            hL->addWidget(w3);
            
            hL->addStretch();
        }
        vL->addLayout(hL);

        vL->addStretch();
    }
}


