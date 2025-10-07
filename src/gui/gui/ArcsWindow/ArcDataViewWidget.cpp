#include "ArcDataViewWidget.h"

#include <Widgets/IconButton.h>

#include <QScrollArea>
#include <QVBoxLayout>

ArcDataViewWidget::ArcDataViewWidget(QWidget *parent) noexcept
    : QWidget(parent)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            IconButton *btn = new IconButton(this, ":/arrow.left");
            connect(btn, &IconButton::clicked, [this] { emit go_back(); });
            btn->setBgColor("#8a8a8a");
            hL->addWidget(btn);

            hL->addStretch();
        }
        vL->addLayout(hL);

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setWidgetResizable(true);
        vL->addWidget(scrollArea_);
    }
}
