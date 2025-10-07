#include "MnemonicWindow.h"
#include "MnemonicWidget.h"
#include "LegendWidget.h"

#include <Widgets/RoundButton.h>
#include <SysCfgWindow/LockCtrlWindow/LockCtrlWindow.h>
#include <Widgets/ValueSetBool.h>

#include <QVBoxLayout>
#include <QLabel>

#include "global.h"

#include <aem/log.h>

#include <filesystem>

MnemonicWindow::MnemonicWindow(QWidget* parent) noexcept
    : QStackedWidget(parent)
{
    std::filesystem::path path("mimic");

    // Загружаем необходимые изображения
    struct
    {
        char const* fname;
        int id;

    } imgs[] = {
        { "valve-opened.png",   MnemonicImages::valve_opened },
        { "valve-closed.png",   MnemonicImages::valve_closed },
        { "faucet.png",         MnemonicImages::faucet },
        { "rfaucet.png",        MnemonicImages::rfaucet },
        { "shower-off.png",     MnemonicImages::shower_off },
        { "shower-half.png",    MnemonicImages::shower_half },
        { "shower-max.png",     MnemonicImages::shower_max },
        { "slevel-on.png",      MnemonicImages::slevel_on },
        { "slevel-off.png",     MnemonicImages::slevel_off },
        { "heater-on.png",      MnemonicImages::heater_on },
        { "heater-off.png",     MnemonicImages::heater_off },
        { "pump-on.png",        MnemonicImages::pump_on },
        { "pump-off.png",       MnemonicImages::pump_off },
    };

    for (auto const& img : imgs)
        images_[img.id].load((path / img.fname).c_str());

    QWidget *w = new QWidget(this);
    
    mW_ = new MnemonicWidget(w, images_);
        
    QVBoxLayout *vL = new QVBoxLayout(w);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            QLabel *lbl = new QLabel(this);
            lbl->setText("Мнемосхема");
            lbl->setStyleSheet("color: #8A8A8A; font-size: 16pt");
            hL->addWidget(lbl);

            hL->addStretch();
            
            for (std::size_t page = 0; page < mW_->pages(); ++page)
            {
                RoundButton *btn = new RoundButton(this);
                btn->setText(QString::number(page + 1));
                connect(btn, &RoundButton::clicked, [this, page] {
                    switch_mimic_page(page);
                });
                hL->addWidget(btn);
            }
            
            hL->addStretch();

            RoundButton *btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { setCurrentIndex(1); });
            btn->setBgColor("#29AC39");
            btn->setTextColor(Qt::white);
            btn->setText("Уставки контроля");
            hL->addWidget(btn);

            hL->addSpacing(50);

            info_ = new ValueSetBool(this);
            connect(info_, &ValueSetBool::onValueChanged, [this] { mW_->show_info(info_->value()); });
            info_->setMaximumWidth(200);
            info_->setValueView("Единицы измерения", "Идентификатор");
            info_->setTitle("Информация");
            hL->addWidget(info_);
        }
        vL->addLayout(hL);

        vL->addWidget(mW_);
        
        LegendWidget *lw = new LegendWidget(w, images_);
        vL->addWidget(lw);
    }
    vL->setStretch(1, 1);
    QStackedWidget::addWidget(w);

    lcW_ = new LockCtrlWindow(this);
    lcW_->add_ctl("Нагрев", "fc");
    lcW_->add_ctl("Спрейер", "sprayer");
    lcW_->add_ctl("Привод", "moving");
    connect(lcW_, &LockCtrlWindow::show_mimic, [this] { setCurrentIndex(0); });
    QStackedWidget::addWidget(lcW_);

    global::subscribe("{sensor,flag}.{}", [this, w](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    {
        std::string const &type = keys[0].get_ref<std::string const&>();
        std::string const &name = keys[1].get_ref<std::string const&>();
        mW_->update_sensor_value(fmt::format("{}.{}", type, name), value.get<float>());
    });

    global::subscribe("valve.{}", [this, w](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    {
        std::string const& key = keys[0].get_ref<std::string const&>();
        mW_->update_valve_state(key, value.get<bool>());
    });
    
    global::subscribe("slevel.{}", [this, w](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    {
        std::string const& key = keys[0].get_ref<std::string const&>();
        mW_->update_slevel_state(key, value.get<bool>());
    });
    
    global::subscribe("pump.{}", [this, w](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    {
        std::string const& key = keys[0].get_ref<std::string const&>();
        mW_->update_pump_state(key, value.get<bool>());
    });
    
    global::subscribe("heater.{}", [this, w](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    {
        std::string const& key = keys[0].get_ref<std::string const&>();
        mW_->update_heater_state(key, value.get<bool>());
    });

    global::subscribe("locker.{sensor,flag}.{}", [this, w](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    {
        std::string const &type = keys[0].get_ref<std::string const&>();
        std::string const &name = keys[1].get_ref<std::string const&>();
        
        auto status = value.get<we::locker::item_status>();
        mW_->update_sensor_state(fmt::format("{}.{}", type, name), status);
    });
}

void MnemonicWindow::switch_mimic_page(std::size_t page)
{
    mW_->switch_to(page);
}
