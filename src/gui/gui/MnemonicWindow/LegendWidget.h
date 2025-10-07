#pragma once

#include <QWidget>

struct MnemonicImages;

class LegendWidget final
    : public QWidget
{
public:

    LegendWidget(QWidget*, MnemonicImages const&) noexcept;

};


