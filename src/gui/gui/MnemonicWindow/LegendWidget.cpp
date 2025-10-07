#include "LegendWidget.h"
#include "MnemonicCommon.h"

#include <QHBoxLayout>
#include <QLabel>

LegendWidget::LegendWidget(QWidget *parent, MnemonicImages const& images) noexcept
    : QWidget(parent)
{
    QFont title_font = font();
    title_font.setPixelSize(12);

    QHBoxLayout *hL = new QHBoxLayout(this);
    {
        MnemonicInfo::desc const* sensors[] = 
            { &MnemonicInfo::pressure(), &MnemonicInfo::temperature(), &MnemonicInfo::inflow() };

        for (auto const& sens : sensors)
        {
            QLabel *lbl = new QLabel(sens->units, this);
            lbl->setStyleSheet(QString("background-color: %1; border-radius: 10px;padding: 5px;").arg(sens->color.name()));
            hL->addWidget(lbl);

            lbl = new QLabel(sens->title, this);
            lbl->setFont(title_font);
            hL->addWidget(lbl);

            hL->addStretch();
        }

        struct 
        {
            int id;
            QString title;

        } const elements[] {
            { MnemonicImages::valve_closed, "Клапан" },
            { MnemonicImages::faucet,       "Кран" },
            { MnemonicImages::rfaucet,      "Рег. кран" },
            { MnemonicImages::shower_max,   "Душ" },
            { MnemonicImages::slevel_off,   "Датчик" },
        };

        for (auto const& e : elements)
        {
            hL->addStretch();

            QLabel *lbl = new QLabel(this);
            lbl->setPixmap(images[e.id]);
            hL->addWidget(lbl);

            lbl = new QLabel(e.title, this);
            lbl->setFont(title_font);
            hL->addWidget(lbl);
        }
    }

}

