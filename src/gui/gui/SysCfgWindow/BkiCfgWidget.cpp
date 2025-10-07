#include "BkiCfgWidget.h"

#include <Widgets/ValueSetBool.h>

#include <QVBoxLayout>
#include <QLabel>

#include <defs.h>

#include "global.h"

BkiCfgWidget::BkiCfgWidget(QWidget *parent)
    : QWidget(parent)
    , rpc_(global::rpc())
{
    QHBoxLayout* hL = new QHBoxLayout(this);
    hL->setContentsMargins(20, 20, 20, 20);
    hL->setSpacing(20);
    {
        hL->addStretch();

        QVBoxLayout *vL = new QVBoxLayout();
        {
            vL->addSpacing(50);

            QFont f(QWidget::font());
            f.setPointSize(16);

            QLabel *lbl = new QLabel();
            lbl->setStyleSheet("color: #e55056");
            lbl->setFont(f);
            lbl->setText(R"V0G0N(
                Данная настройка выключает контроль касания индуктора и заготовки. 

                Выключение контроля может привести к физической деформации индуктора 
                о заготовку при некоректной траектории движения. 

                Функции автоматического позиционирования будут недоступны.

                Для применения изменений необходима перезагрузка.)V0G0N");
            vL->addWidget(lbl);

            vsb_ = new ValueSetBool(this);
            vsb_->setValueView("ВЫКЛ", "ВКЛ");
            vsb_->setTitle("БКИ");
            vsb_->setMaximumWidth(80);
            vsb_->setMinimumWidth(80);
            connect(vsb_, &ValueSetBool::onValueChanged, [this] { on_bki_status(); });
            vL->addWidget(vsb_);

            vL->setAlignment(vsb_, Qt::AlignCenter);

            vL->addStretch();
        }
        hL->addLayout(vL);

        hL->addStretch();
    }

    global::subscribe("sys.bki-allow", [this](nlohmann::json::array_t const&, nlohmann::json const& value)
    {
        vsb_->setJsonValue(value);
    });
}

void BkiCfgWidget::on_bki_status()
{
    rpc_.call("set", { "bki", "allow", vsb_->value() });
}
