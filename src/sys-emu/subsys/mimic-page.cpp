#include "mimic-page.hpp"

#include <Widgets/RoundButton.h>

#include "listeners/base-device.hpp"
#include "listeners/syslink.hpp"
#include "listeners/devices/PR205-B2.hpp"
#include "listeners/devices/PR205-B6.hpp"
#include "listeners/devices/PR205-B10.hpp"
#include "listeners/devices/PR205-A14.hpp"
#include "listeners/devices/PR205-A1.hpp"
#include "listeners/devices/PR200.hpp"

#include <QHBoxLayout>
#include <QGroupBox>
#include <QTimer>
#include <QSettings>

mimic_page::mimic_page(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hL = new QHBoxLayout(this);
    {
        QGroupBox *box = new QGroupBox(this);
        box->setTitle("Насосы");
        QVBoxLayout *vL = new QVBoxLayout(box);
        {
            for (std::size_t i = 0; i < 7; ++i)
            {
                std::string key = std::format("H{}", i + 1);

                RoundButton *btn = new RoundButton(box);
                btn->setText(QString::fromStdString(key));
                init_H(key, btn);
                vL->addWidget(btn);
            }
        }
        hL->addWidget(box);

        box = new QGroupBox(this);
        box->setTitle("Клапана");
        vL = new QVBoxLayout(box);
        {
            std::string_view const keys[] =
            {
                "VA1", "VA4", "VA5", "VA6", "VA7", "VA8",
                "VA9", "VA13", "VA14", "VA15"
            };

            for (std::size_t i = 0; i < std::size(keys); ++i)
            {
                RoundButton *btn = new RoundButton(box);
                btn->setText(QString::fromUtf8(keys[i].data(), keys[i].length()));
                init_VA(keys[i], btn);
                vL->addWidget(btn);
            }
        }
        hL->addWidget(box);

        box = new QGroupBox(this);
        box->setTitle("Датчики уровня");
        vL = new QVBoxLayout(box);
        {
            for (std::size_t i = 0; i < 11; ++i)
            {
                std::string key = std::format("P{}", i + 1);

                RoundButton *btn = new RoundButton(box);
                btn->setText(QString::fromStdString(key));
                init_P(key, btn);
                vL->addWidget(btn);
            }
        }
        hL->addWidget(box);

        box = new QGroupBox(this);
        box->setTitle("Задвижки позиционные");
        vL = new QVBoxLayout(box);
        {
            std::string_view const keys[] = {
                "VA10", "VA11", "VA12"
            };

            for (std::size_t i = 0; i < std::size(keys); ++i)
            {
                QHBoxLayout *hL = new QHBoxLayout();
                {
                    std::string lkey = std::format("l-{}", keys[i]);

                    auto vL = new QVBoxLayout();
                    {
                        QLabel *lbl = new QLabel(box);
                        lbl->setAlignment(Qt::AlignCenter);
                        lbl->setFrameStyle(QFrame::Box);
                        lbl->setFixedHeight(25);
                        init_VA(lkey, lbl);
                        vL->addWidget(lbl);

                        RoundButton *btn = new RoundButton(box);
                        btn->setText(QString::fromStdString(lkey));
                        init_VA(lkey, btn, true);
                        load_state(btn);
                        vL->addWidget(btn);
                    }
                    hL->addLayout(vL);

                    std::string rkey = std::format("r-{}", keys[i]);

                    vL = new QVBoxLayout();
                    {
                        QLabel *lbl = new QLabel(box);
                        lbl->setAlignment(Qt::AlignCenter);
                        lbl->setFrameStyle(QFrame::Box);
                        lbl->setFixedHeight(25);
                        init_VA(rkey, lbl);
                        vL->addWidget(lbl);

                        RoundButton *btn = new RoundButton(box);
                        btn->setText(QString::fromStdString(rkey));
                        init_VA(rkey, btn, true);
                        load_state(btn);
                        vL->addWidget(btn);
                    }
                    hL->addLayout(vL);
                }
                vL->addLayout(hL);
            }
        }
        hL->addWidget(box);
    }
}

void mimic_page::update_btn_view(RoundButton *btn, bool value)
{
    btn->setTextColor(value ? Qt::white : Qt::black);
    btn->setBgColor(value ? QColor(iw::color::green) : Qt::white);
}

template <typename T>
static bool dev_flip(std::size_t ibit)
{
    bool value = syslink::device<T>().bit_get(ibit);
    value = !value;
    syslink::device<T>().bit_set(ibit, value);
    return value;
}

template <typename T>
static bool dev_p_flip(std::size_t ibit)
{
    bool value = syslink::device<T>().bit_p_get(ibit);
    value = !value;
    syslink::device<T>().bit_p_set(ibit, value);
    return value;
}

template <typename T>
static void dev_p_notify(std::size_t ibit, devices::bitset_callback callback)
{
    syslink::device<T>().bitset_p(ibit, std::move(callback));
}

template <typename T>
static void dev_notify(std::size_t ibit, devices::bitset_callback callback)
{
    syslink::device<T>().bitset(ibit, std::move(callback));
}

template <typename T>
static bool dev_va_flip(std::size_t ibit)
{
    bool value = syslink::device<T>().bit_va_get(ibit);
    value = !value;
    syslink::device<T>().bit_va_set(ibit, value);
    return value;
}

template <typename T>
static void dev_va_notify(std::size_t ibit, devices::bitset_callback callback)
{
    syslink::device<T>().bitset_va(ibit, std::move(callback));
}


void mimic_page::init_H(std::string const &key, RoundButton *btn)
{
    struct item_t
    {
        bool(*flip)(std::size_t);
        void(*notify)(std::size_t, devices::bitset_callback);
        std::size_t ibit;
    };

    static std::unordered_map<std::string_view, item_t> const map {
        { "H1",  { dev_flip<devices::PR205_B2>,  dev_notify<devices::PR205_B2>,  0 } },
        { "H2",  { dev_flip<devices::PR205_B2>,  dev_notify<devices::PR205_B2>,  1 } },
        { "H3",  { dev_flip<devices::PR205_B6>,  dev_notify<devices::PR205_B6>,  0 } },
        { "H4",  { dev_flip<devices::PR205_B6>,  dev_notify<devices::PR205_B6>,  1 } },
        { "H5",  { dev_flip<devices::PR205_B10>, dev_notify<devices::PR205_B10>, 0 } },
        { "H6",  { dev_flip<devices::PR205_B10>, dev_notify<devices::PR205_B10>, 1 } },
        { "H7",  { dev_flip<devices::PR205_A14>, dev_notify<devices::PR205_A14>, 0 } },
    };

    auto const &item = map.at(key);

    connect(btn, &RoundButton::clicked, [this, btn, &item]
    {
        bool value = item.flip(item.ibit);
        update_btn_view(btn, value);
    });

    item.notify(item.ibit, [this, btn](bool value)
    {
        update_btn_view(btn, value);
    });
}

void mimic_page::init_P(std::string const &key, RoundButton *btn)
{
    struct item_t
    {
        bool(*flip)(std::size_t);
        std::size_t ibit;
    };

    static std::unordered_map<std::string_view, item_t> const map {
        { "P1",  { dev_p_flip<devices::PR205_A14>, 2 } },
        { "P2",  { dev_p_flip<devices::PR205_A14>, 3 } },
        { "P3",  { dev_p_flip<devices::PR205_A14>, 0 } },
        { "P4",  { dev_p_flip<devices::PR205_A14>, 1 } },
        { "P5",  { dev_p_flip<devices::PR205_A14>, 4 } },
        { "P6",  { dev_flip<devices::PR205_B2>,  4 } },
        { "P7",  { dev_flip<devices::PR205_B2>,  5 } },
        { "P8",  { dev_flip<devices::PR205_B6>,  4 } },
        { "P9",  { dev_flip<devices::PR205_B6>,  5 } },
        { "P10", { dev_flip<devices::PR205_B10>, 4 } },
        { "P11", { dev_flip<devices::PR205_B10>, 5 } },
    };

    auto const &item = map.at(key);

    connect(btn, &RoundButton::clicked, [this, btn, &item]
    {
        bool value = item.flip(item.ibit);
        update_btn_view(btn, value);
    });
}

void mimic_page::init_VA(std::string_view key, RoundButton *btn, bool save)
{
    struct item_t
    {
        bool(*flip)(std::size_t);
        void(*notify)(std::size_t, devices::bitset_callback);
        std::size_t ibit;
    };

    static std::unordered_map<std::string_view, item_t> const map {
        { "VA1",    { dev_flip<devices::PR200>,     dev_notify<devices::PR200>,     7 } },
        { "VA4",    { dev_flip<devices::PR205_B2>,  dev_notify<devices::PR205_B2>,  3 } },
        { "VA5",    { dev_flip<devices::PR205_B2>,  dev_notify<devices::PR205_B2>,  4 } },
        { "VA6",    { dev_flip<devices::PR205_B6>,  dev_notify<devices::PR205_B6>,  3 } },
        { "VA7",    { dev_flip<devices::PR205_B6>,  dev_notify<devices::PR205_B6>,  4 } },
        { "VA8",    { dev_flip<devices::PR205_B10>, dev_notify<devices::PR205_B10>, 3 } },
        { "VA9",    { dev_flip<devices::PR205_B10>, dev_notify<devices::PR205_B10>, 4 } },
        { "l-VA10", { dev_va_flip<devices::PR205_A14>, dev_va_notify<devices::PR205_A14>, 0 } },
        { "r-VA10", { dev_va_flip<devices::PR205_A14>, dev_va_notify<devices::PR205_A14>, 1 } },
        { "l-VA11", { dev_va_flip<devices::PR205_A14>, dev_va_notify<devices::PR205_A14>, 2 } },
        { "r-VA11", { dev_va_flip<devices::PR205_A14>, dev_va_notify<devices::PR205_A14>, 3 } },
        { "l-VA12", { dev_va_flip<devices::PR205_A14>, dev_va_notify<devices::PR205_A14>, 4 } },
        { "r-VA12", { dev_va_flip<devices::PR205_A14>, dev_va_notify<devices::PR205_A14>, 5 } },
        { "VA13",   { dev_flip<devices::PR205_A1>,  dev_notify<devices::PR205_A1>,  0 } },
        { "VA14",   { dev_flip<devices::PR205_A1>,  dev_notify<devices::PR205_A1>,  1 } },
        { "VA15",   { dev_flip<devices::PR205_A1>,  dev_notify<devices::PR205_A1>,  2 } },
    };

    auto const &item = map.at(key);

    connect(btn, &RoundButton::clicked, [this, btn, &item, save]
    {
        bool value = item.flip(item.ibit);
        update_btn_view(btn, value);

        if (save)
        {
            QSettings settings("sys-emu");
            settings.setValue(btn->text(), value);
        }
    });

    item.notify(item.ibit, [this, btn](bool value)
    {
        update_btn_view(btn, value);
    });
}

void mimic_page::init_VA(std::string_view key, QLabel *lbl)
{
    struct item_t
    {
        void(*notify)(std::size_t, devices::bitset_callback);
        std::size_t ibit;
    };

    static std::unordered_map<std::string_view, item_t> const map {
        { "l-VA10", { dev_notify<devices::PR205_A14>, 1 } },
        { "r-VA10", { dev_notify<devices::PR205_A14>, 2 } },
        { "l-VA11", { dev_notify<devices::PR205_A14>, 3 } },
        { "r-VA11", { dev_notify<devices::PR205_A14>, 4 } },
        { "l-VA12", { dev_notify<devices::PR205_A14>, 5 } },
        { "r-VA12", { dev_notify<devices::PR205_A14>, 6 } },
    };

    auto const &item = map.at(key);

    item.notify(item.ibit, [this, lbl](bool value)
    {
        lbl->setStyleSheet(value ? "background-color: green; color: white" : "");
    });
}

void mimic_page::load_state(RoundButton *btn)
{
    static std::unordered_map<std::string_view, std::size_t> const map {
        { "l-VA10", 0 },
        { "r-VA10", 1 },
        { "l-VA11", 2 },
        { "r-VA11", 3 },
        { "l-VA12", 4 },
        { "r-VA12", 5 },
    };

    std::size_t ibit = map.at(btn->text().toLocal8Bit().constData());

    QTimer::singleShot(0, [this, btn, ibit]
    {
        QSettings settings("sys-emu");
        bool value = settings.value(btn->text(), false).toBool();
        syslink::device<devices::PR205_A14>().bit_va_set(ibit, value);
        update_btn_view(btn, value);
    });
}

