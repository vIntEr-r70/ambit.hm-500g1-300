#include "SysSetsWidget.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>

#include <Widgets/ValueSetString.h>
#include <Widgets/ValueSetInt.h>

#include <eng/log.hpp>

SysSetsWidget::SysSetsWidget(QWidget* parent) noexcept
    : QWidget(parent)
{
    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->addStretch();
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            hL->addStretch();

            QVBoxLayout* vL = new QVBoxLayout();
            {
                vss_unit_name_ = new ValueSetString(this, 0);
                vss_unit_name_->setTitle("Название установки");
                vL->addWidget(vss_unit_name_);

                vL->addSpacing(20);

                vss_unit_sid_ = new ValueSetString(this, lIdUnitId);
                vss_unit_sid_->setTitle("Идентификатор установки");
                vL->addWidget(vss_unit_sid_);

                vL->addSpacing(20);

                QGroupBox *gb = new QGroupBox(this);
                gb->setStyleSheet("color: #696969; font-weight: 500");
                gb->setTitle("Время");
                gb->setAlignment(Qt::AlignCenter);
                QHBoxLayout* hhL = new QHBoxLayout(gb);
                {
                    vsi_hour_ = new ValueSetInt(this);
                    connect(vsi_hour_, &ValueSetInt::onValueChanged, [this] { on_value_changed(); });
                    vsi_hour_->setTitle("Часы");
                    vsi_hour_->setAlignment(Qt::AlignCenter);
                    hhL->addWidget(vsi_hour_);

                    vsi_mins_ = new ValueSetInt(this);
                    connect(vsi_mins_, &ValueSetInt::onValueChanged, [this] { on_value_changed(); });
                    vsi_mins_->setTitle("Минуты");
                    vsi_mins_->setAlignment(Qt::AlignCenter);
                    hhL->addWidget(vsi_mins_);
                }
                vL->addWidget(gb);

                vL->addSpacing(20);

                gb = new QGroupBox(this);
                gb->setStyleSheet("color: #696969; font-weight: 500");
                gb->setTitle("Дата");
                gb->setAlignment(Qt::AlignCenter);
                hhL = new QHBoxLayout(gb);
                {
                    vsi_day_ = new ValueSetInt(this);
                    connect(vsi_day_, &ValueSetInt::onValueChanged, [this] { on_value_changed(); });
                    vsi_day_->setTitle("День");
                    vsi_day_->setAlignment(Qt::AlignCenter);
                    hhL->addWidget(vsi_day_);

                    vsi_month_ = new ValueSetInt(this);
                    connect(vsi_month_, &ValueSetInt::onValueChanged, [this] { on_value_changed(); });
                    vsi_month_->setTitle("Месяц");
                    vsi_month_->setAlignment(Qt::AlignCenter);
                    hhL->addWidget(vsi_month_);

                    vsi_year_ = new ValueSetInt(this);
                    connect(vsi_year_, &ValueSetInt::onValueChanged, [this] { on_value_changed(); });
                    vsi_year_->setTitle("Год");
                    vsi_year_->setAlignment(Qt::AlignCenter);
                    hhL->addWidget(vsi_year_);
                }
                vL->addWidget(gb);
            }
            hL->addLayout(vL);

            hL->addStretch();
        }
        vL->addLayout(hL);

        vL->addStretch();

        hL = new QHBoxLayout();
        {
            QGridLayout *gL = new QGridLayout();
            {
                QLabel *lbl;

                lbl = new QLabel("Наработка станка, часов:");
                lbl->setStyleSheet("font-size: 10pt; color: #696969;");
                gL->addWidget(lbl, 0, 0);

                lbl = new QLabel("Наработка ПЧ, часов:");
                lbl->setStyleSheet("font-size: 10pt; color: #696969;");
                gL->addWidget(lbl, 1, 0);

                cn_optime_ = new QLabel(this);
                cn_optime_->setStyleSheet("font-size: 10pt; color: #696969;");
                cn_optime_->setMinimumWidth(150);
                gL->addWidget(cn_optime_, 0, 1);

                fc_optime_ = new QLabel(this);
                fc_optime_->setStyleSheet("font-size: 10pt; color: #696969;");
                fc_optime_->setMinimumWidth(150);
                gL->addWidget(fc_optime_, 1, 1);
            }
            hL->addLayout(gL);

            hL->addStretch();
        }
        vL->addLayout(hL);
    }

    // sys_key_map_.add("cn", this, &SysSetsWidget::nf_sys_optime_cn);
    // sys_key_map_.add("fc", this, &SysSetsWidget::nf_sys_optime_fc);
    //
    // global::subscribe("sys.optime.{}", [this](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    // {
    //     std::string_view kparam = keys[0].get<std::string_view>();
    //     sys_key_map_.apply(kparam, value);
    // });
}

void SysSetsWidget::nf_sys_optime_fc(std::size_t sec) {
    fc_optime_->setText(QString::number(sec / 3600));
}

void SysSetsWidget::nf_sys_optime_cn(std::size_t sec) {
    cn_optime_->setText(QString::number(sec / 3600));
}

void SysSetsWidget::set_guid(int guid)
{
    // vss_unit_name_->setReadOnly(guid != auth_manufacturer);
    // vss_unit_sid_->setReadOnly(guid != auth_manufacturer);
}

void SysSetsWidget::initNotify()
{
    // SysCfg::inst().coreMap["cfg"]["gui"].getValue("main-title").connect([this](Ex::Acs::value const& v)
    // {
    //     std::string ttl(v.asString());
    //     auto pos = ttl.find('\n');
    //     mainTitleBtn_[0]->setText(QString::fromStdString(ttl.substr(0, pos)));
    //     if (pos != std::string::npos)
    //         mainTitleBtn_[1]->setText(QString::fromStdString(ttl.substr(pos + 1)));
    // });
    //
    // SysCfg::inst().coreMap["cfg"].getValue("unit-id").connect([this](Ex::Acs::value const& v)
    // {
    //     btnUnitIsStr_->setText(QString::fromStdString(v.asString()));
    // });
}

void SysSetsWidget::showEvent(QShowEvent*)
{
    QDateTime dt(QDateTime::currentDateTime());

    QDate date(dt.date());
    vsi_day_->set_value(date.day());
    vsi_month_->set_value(date.month());
    vsi_year_->set_value(date.year());
    
    QTime time(dt.time());
    vsi_hour_->set_value(time.hour());
    vsi_mins_->set_value(time.minute());
}

void SysSetsWidget::on_value_changed()
{
    char buf[128];
    std::snprintf(buf, sizeof(buf), "date --set \"%04d-%02d-%02d %02d:%02d\"",
            vsi_year_->value(), vsi_month_->value(), vsi_day_->value(), 
            vsi_hour_->value(), vsi_mins_->value());

    eng::log::warn("Системное время: {}", buf);

#ifdef BULDROOT
    std::system(buf);
    std::system("hwclock -w");
#endif
}

void SysSetsWidget::makeApplyUnitId()
{
    // netRpc_.reset();
    // netRpc_.add("cfg/unit-id");
    // netRpc_.add(btnUnitIsStr_->text().toLocal8Bit().constData());
    // netRpc_.call("cfg");
}

void SysSetsWidget::makeApplyMainTitle()
{
    // std::string ttl(mainTitleBtn_[0]->text().toLocal8Bit().constData());
    // ttl += "\n";
    // ttl += mainTitleBtn_[1]->text().toLocal8Bit().constData();
    //
    // netRpc_.reset();
    // netRpc_.add("cfg/gui/main-title");
    // netRpc_.add(ttl);
    // netRpc_.call("cfg");
}
