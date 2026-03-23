#include "main-frame.hpp"
#include "subsys/panel-page.hpp"
#include "subsys/sens-fc-page.hpp"
#include "subsys/sens-dt-page.hpp"
#include "subsys/sens-dp-page.hpp"
#include "subsys/frequency-converter-page.hpp"
#include "subsys/mimic-page.hpp"

#include <QKeyEvent>

main_frame::main_frame()
    : QTabWidget(nullptr)
{
    QFont f(font());
    f.setPointSize(16);
    setFont(f);

    addTab(new panel_page(this), "Панель");
    addTab(new sens_fc_page(this), "Проток");
    addTab(new sens_dt_page(this), "Температура");
    addTab(new sens_dp_page(this), "Давление");
    addTab(new frequency_converter_page(this), "ПЧ");
    addTab(new mimic_page(this), "Мнемосхема");
}

void main_frame::keyPressEvent(QKeyEvent *e) noexcept
{
    if (e->key() == Qt::Key_Escape)
        hide();
}
