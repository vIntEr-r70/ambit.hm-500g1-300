#pragma once

#include <QTabWidget>

class main_frame final
    : public QTabWidget
{
public:

    main_frame();

private:

    void keyPressEvent(QKeyEvent*) noexcept override final;
};
