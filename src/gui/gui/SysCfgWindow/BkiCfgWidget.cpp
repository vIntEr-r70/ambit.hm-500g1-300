#include "BkiCfgWidget.h"

#include <eng/sibus/client.hpp>
#include <eng/json.hpp>
#include <eng/json/builder.hpp>

#include <Widgets/ValueSetBool.h>

#include <QVBoxLayout>
#include <QLabel>

BkiCfgWidget::BkiCfgWidget(QWidget *parent)
    : QWidget(parent)
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
                о заготовку при некорректной траектории движения.

                Функции автоматического позиционирования будут недоступны.)V0G0N");
            vL->addWidget(lbl);

            vsb_ = new ValueSetBool(this);
            vsb_->setValueView("ВЫКЛ", "ВКЛ");
            vsb_->set_value(true);
            vsb_->setTitle("БКИ");
            vsb_->setFixedWidth(80);
            connect(vsb_, &ValueSetBool::onValueChanged, [this] { on_bki_status(); });
            vL->addWidget(vsb_);

            vL->setAlignment(vsb_, Qt::AlignCenter);

            vL->addStretch();
        }
        hL->addLayout(vL);

        hL->addStretch();
    }

    load_config();
}

void BkiCfgWidget::on_bki_status()
{
    save_config();
}

void BkiCfgWidget::load_config()
{
    eng::sibus::client::config_listener("bki", [this](std::string_view json)
    {
        // Сохраняем дефолтную конфигурацию
        if (json.empty())
            return;
        eng::json::value cfg(json);
        vsb_->setJsonValue(cfg.get<bool>());
    });
}

void BkiCfgWidget::save_config()
{
    eng::json::builder_t jb;
    eng::json::add_value<bool>(jb, vsb_->value());
    eng::sibus::client::configure("bki", jb.view());
}

