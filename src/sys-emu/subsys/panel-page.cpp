#include "panel-page.hpp"

#include "listeners/syslink.hpp"
#include "listeners/devices/PLC110.hpp"
#include "listeners/devices/FC.hpp"

#include "widgets/pair-buttons.hpp"
#include "widgets/moving-buttons.hpp"
#include "widgets/select-buttons.hpp"
#include "widgets/titled-led.hpp"

#include <defines.hpp>

#include <QVBoxLayout>
#include <QGroupBox>

panel_page::panel_page(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hL = new QHBoxLayout(this);
    {
        QVBoxLayout *vL = new QVBoxLayout();
        {
            vL->addStretch();

            QGridLayout *gL = new QGridLayout();
            {
                auto pb = new pair_buttons(this, "Насос\nОхлажд. ТВЧ");
                gL->addWidget(pb, 1, 0);
                init_pump_fc(pb);

                pb = new pair_buttons(this, "Насос\nСпрейер");
                gL->addWidget(pb, 1, 1);
                init_pump_sprayer(pb);
            }
            vL->addLayout(gL);
        }
        hL->addLayout(vL);

        hL->addStretch();

        vL = new QVBoxLayout();
        {
            QHBoxLayout *hL = new QHBoxLayout();
            {
                QVBoxLayout *vL = new QVBoxLayout();
                {
                    QHBoxLayout *hL = new QHBoxLayout();
                    {
                        select_buttons *sb = new select_buttons(this, "plc/environment", "Среда", { "Закал. жид.", "Вода" });
                        hL->addWidget(sb);
                        init_environment_select(sb);

                        sb = new select_buttons(this, "plc/drainage", "Слив", { "Емкость", "Канал." });
                        hL->addWidget(sb);
                        init_drainage_select(sb);

                        hL->addStretch();

                        titled_led *tl = new titled_led(this, iw::color::red, "AВАРИЯ");
                        hL->addWidget(tl);
                        hL->setAlignment(tl, Qt::AlignTop);
                    }
                    vL->addLayout(hL);

                    vL->addStretch();
                }
                hL->addLayout(vL);

                hL->addStretch();

                pair_buttons *pb = new pair_buttons(this, "АВАРИЙНЫЙ\nСТОП", false);
                hL->addWidget(pb);
                init_emergency_stop(pb);
            }
            vL->addLayout(hL);

            vL->addStretch();

            hL = new QHBoxLayout();
            {
                QGroupBox *gb = new QGroupBox(this);
                gb->setTitle("КАРЕТКА");
                gb->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
                {
                    QVBoxLayout *vL = new QVBoxLayout(gb);
                    {
                        moving_buttons *mb = new moving_buttons(this, "X");
                        vL->addWidget(mb);

                        mb = new moving_buttons(this, "Y");
                        vL->addWidget(mb);

                        mb = new moving_buttons(this, "Z");
                        vL->addWidget(mb);
                    }
                }
                hL->addWidget(gb);

                QVBoxLayout *vL = new QVBoxLayout();
                {
                    QHBoxLayout *hL = new QHBoxLayout();
                    {
                        moving_buttons *mb = new moving_buttons(this, "V");
                        hL->addWidget(mb);

                        select_buttons *sb = new select_buttons(this, "plc/rcu", "Внешний пульт", { "Вкл.", "Выкл." });
                        hL->addWidget(sb);
                        init_rcu_select(sb);
                    }
                    vL->addLayout(hL);

                    select_buttons *sb = new select_buttons(this, "plc/mode", "", { "Руч.", "Авто" });
                    vL->addWidget(sb);
                    init_mode_select(sb);

                    sb = new select_buttons(this, "plc/speed", "Скорость", { "1", "2", "3" });
                    vL->addWidget(sb);
                    init_speed_select(sb);
                }
                hL->addLayout(vL);
            }
            vL->addLayout(hL);

            vL->addStretch();

            hL = new QHBoxLayout();
            {
                pair_buttons *pb = new pair_buttons(this, "НАГРЕВ", false);
                hL->addWidget(pb);
                init_multi_button(pb);

                pb = new pair_buttons(this, "Спрейер\nвода 1");
                hL->addWidget(pb);
                init_sprayer_0(pb);

                pb = new pair_buttons(this, "Спрейер\nвода 2");
                hL->addWidget(pb);
                init_sprayer_1(pb);

                pb = new pair_buttons(this, "Спрейер\nвоздух 1");
                hL->addWidget(pb);
                init_sprayer_2(pb);
            }
            vL->addLayout(hL);
        }
        hL->addLayout(vL);
    }
}

void panel_page::init_pump_fc(pair_buttons *pb)
{
    connect(pb, &pair_buttons::turn_on_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(23, true);
    });
    connect(pb, &pair_buttons::turn_on_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(23, false);
    });
    connect(pb, &pair_buttons::turn_off_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(24, false);
    });
    connect(pb, &pair_buttons::turn_off_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(24, true);
    });

    syslink::device<devices::PLC110>().plc_bitset(8, [pb](bool value)
    {
        pb->led(value);
    });
}

void panel_page::init_pump_sprayer(pair_buttons *pb)
{
    connect(pb, &pair_buttons::turn_on_pressed, [this] {
        syslink::device<devices::PLC110>().m2_bitset(1, true);
    });
    connect(pb, &pair_buttons::turn_on_released, [this] {
        syslink::device<devices::PLC110>().m2_bitset(1, false);
    });
    connect(pb, &pair_buttons::turn_off_pressed, [this] {
        syslink::device<devices::PLC110>().m2_bitset(2, false);
    });
    connect(pb, &pair_buttons::turn_off_released, [this] {
        syslink::device<devices::PLC110>().m2_bitset(2, true);
    });

    syslink::device<devices::PLC110>().m2_bitset(1, [pb](bool value)
    {
        pb->led(value);
    });
}

void panel_page::init_emergency_stop(pair_buttons *pb)
{
    connect(pb, &pair_buttons::turn_on_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(29, false);
    });
    connect(pb, &pair_buttons::turn_off_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(29, true);
    });
}

void panel_page::init_multi_button(pair_buttons *pb)
{
    connect(pb, &pair_buttons::turn_on_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(25, true);
    });
    connect(pb, &pair_buttons::turn_on_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(25, false);
    });
    connect(pb, &pair_buttons::turn_off_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(26, false);
    });
    connect(pb, &pair_buttons::turn_off_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(26, true);
    });
}

void panel_page::init_sprayer_0(pair_buttons *pb)
{
    connect(pb, &pair_buttons::turn_on_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(21, true);
    });
    connect(pb, &pair_buttons::turn_on_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(21, false);
    });
    connect(pb, &pair_buttons::turn_off_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(22, false);
    });
    connect(pb, &pair_buttons::turn_off_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(22, true);
    });

    syslink::device<devices::PLC110>().plc_bitset(9, [pb](bool value)
    {
        pb->led(value);
    });
}

void panel_page::init_sprayer_1(pair_buttons *pb)
{
    connect(pb, &pair_buttons::turn_on_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(30, true);
    });
    connect(pb, &pair_buttons::turn_on_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(30, false);
    });
    connect(pb, &pair_buttons::turn_off_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(31, false);
    });
    connect(pb, &pair_buttons::turn_off_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(31, true);
    });

    syslink::device<devices::PLC110>().plc_bitset(10, [pb](bool value)
    {
        pb->led(value);
    });
}

void panel_page::init_sprayer_2(pair_buttons *pb)
{
    connect(pb, &pair_buttons::turn_on_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(34, true);
    });
    connect(pb, &pair_buttons::turn_on_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(34, false);
    });
    connect(pb, &pair_buttons::turn_off_pressed, [this] {
        syslink::device<devices::PLC110>().plc_bitset(35, false);
    });
    connect(pb, &pair_buttons::turn_off_released, [this] {
        syslink::device<devices::PLC110>().plc_bitset(35, true);
    });

    syslink::device<devices::PLC110>().plc_bitset(11, [pb](bool value)
    {
        pb->led(value);
    });
}

void panel_page::init_speed_select(select_buttons *sb)
{
    connect(sb, &select_buttons::selected, [this](QString const &value)
    {
        static std::unordered_map<QString, std::pair<bool, bool>> const map {
            { "1", { false, false } },
            { "2", { true, false } },
            { "3", { false, true } },
        };
        auto const &item = map.at(value);
        syslink::device<devices::PLC110>().plc_bitset(19, item.first);
        syslink::device<devices::PLC110>().plc_bitset(20, item.second);
    });
}

void panel_page::init_mode_select(select_buttons *sb)
{
    connect(sb, &select_buttons::selected, [this](QString const &value)
    {
        static std::unordered_map<QString, bool> const map {
            { "Руч.", false },
            { "Авто", true },
        };
        auto item = map.at(value);
        syslink::device<devices::PLC110>().plc_bitset(27, item);
    });
}

void panel_page::init_rcu_select(select_buttons *sb)
{
    connect(sb, &select_buttons::selected, [this](QString const &value)
    {
        static std::unordered_map<QString, bool> const map {
            { "Вкл.", true },
            { "Выкл.", false },
        };
        auto item = map.at(value);
        syslink::device<devices::PLC110>().plc_bitset(28, item);
    });
}

void panel_page::init_environment_select(select_buttons *sb)
{
    connect(sb, &select_buttons::selected, [this](QString const &value)
    {
        static std::unordered_map<QString, bool> const map {
            { "Закал. жид.", true },
            { "Вода", false },
        };
        auto item = map.at(value);
        syslink::device<devices::PLC110>().m2_bitset(3, item);
    });
}

void panel_page::init_drainage_select(select_buttons *sb)
{
    connect(sb, &select_buttons::selected, [this](QString const &value)
    {
        static std::unordered_map<QString, bool> const map {
            { "Емкость", true },
            { "Канал.", false },
        };
        auto item = map.at(value);
        syslink::device<devices::PLC110>().m2_bitset(4, item);
    });
}

