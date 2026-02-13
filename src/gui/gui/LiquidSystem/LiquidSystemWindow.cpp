#include "LiquidSystemWindow.h"

#include <QuenchMediaWidget/QuenchMediaWidget.h>
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

            auto *w1 = new QuenchMediaWidget(this, "fc", "Вода ТВЧ");
            w1->setFixedWidth(280);
            hL->addWidget(w1);
            auto *w2 = new QuenchMediaWidget(this, "water", "Закалочная вода");
            w2->setFixedWidth(280);
            hL->addWidget(w2);
            auto *w3 = new QuenchMediaWidget(this, "fluid", "Закалочная жидкость");
            w3->setFixedWidth(280);
            hL->addWidget(w3);

            hL->addStretch();
        }
        vL->addLayout(hL);

        vL->addStretch();
    }
}


