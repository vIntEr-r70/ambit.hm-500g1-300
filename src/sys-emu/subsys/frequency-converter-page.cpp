#include "frequency-converter-page.hpp"
#include "widgets/sens-value-set.hpp"

#include "listeners/syslink.hpp"
#include "listeners/devices/FC.hpp"

#include <Widgets/RoundButton.h>
#include <defines.hpp>

#include <QVBoxLayout>

frequency_converter_page::frequency_converter_page(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            QWidget *w = new QWidget(this);
            w->setFixedWidth(600);
            {
                QHBoxLayout *hL = new QHBoxLayout(w);
                {
                    std::pair<QString, std::uint16_t> const list[] {
                        { "U-in",   0xA432 },
                        { "U-out",  0xA433 },
                        { "F",      0xA430 },
                        { "I",      0xA434 },
                        { "P",      0xA437 },
                    };
                    std::ranges::for_each(list, [w, hL](auto item)
                    {
                        auto wi = new sens_value_set(w, item.first);
                        auto reg = item.second;
                        connect(wi, &sens_value_set::change_value, [w, reg](int value) {
                            syslink::device<devices::FC>().set(reg, static_cast<std::int16_t>(value));
                        });
                        hL->addWidget(wi);
                    });
                }
            }
            hL->addWidget(w);

            hL->addStretch();

            QVBoxLayout *vL = new QVBoxLayout();
            {
                vL->addStretch();

                QLabel *lbl = new QLabel(this);
                lbl->setFixedSize(200, 30);
                lbl->setFrameStyle(QFrame::Box);
                lbl->setAlignment(Qt::AlignCenter);
                lbl->setText("РАБОТА");
                lbl_powered_ = lbl;
                vL->addWidget(lbl);

                lbl = new QLabel(this);
                lbl->setFixedSize(200, 30);
                lbl->setFrameStyle(QFrame::Box);
                lbl->setAlignment(Qt::AlignCenter);
                lbl->setText("- - -");
                lbl_I_set_ = lbl;
                vL->addWidget(lbl);

                lbl = new QLabel(this);
                lbl->setFixedSize(200, 30);
                lbl->setFrameStyle(QFrame::Box);
                lbl->setAlignment(Qt::AlignCenter);
                lbl->setText("- - -");
                lbl_U_set_ = lbl;
                vL->addWidget(lbl);

                lbl = new QLabel(this);
                lbl->setFixedSize(200, 30);
                lbl->setFrameStyle(QFrame::Box);
                lbl->setAlignment(Qt::AlignCenter);
                lbl->setText("- - -");
                lbl_P_set_ = lbl;
                vL->addWidget(lbl);

                vL->addStretch();
            }
            hL->addLayout(vL);

            hL->addStretch();
        }
        vL->addLayout(hL);

        QGridLayout *gL = new QGridLayout();
        gL->setHorizontalSpacing(0);
        gL->setVerticalSpacing(0);
        {
            for (std::size_t ireg = 0; ireg < 4; ++ireg)
            {
                for (std::size_t col = 0; col < 16; ++col)
                {
                    std::size_t ibit = 15 - col;

                    RoundButton *btn = new RoundButton(this);
                    btn->setText(QString::number(ibit));
                    connect(btn, &RoundButton::clicked, [this, ireg, ibit] {
                        invert_error_mask_bit(ireg, ibit);
                    });
                    errors_[ireg].buttons[ibit] = btn;
                    gL->addWidget(btn, ireg, col);
                }
            }
        }
        vL->addLayout(gL);
    }

    init_callbacks();
}

void frequency_converter_page::init_callbacks()
{
    syslink::device<devices::FC>().reset_callback_ = [this]
    {
        for (std::size_t ireg = 0; ireg < errors_.size(); ++ireg)
        {
            for (std::size_t i = 0; i < 16; ++i)
                errors_[ireg].buttons[i]->setBgColor(Qt::white);
            errors_[ireg].mask.reset();
        }
    };

    syslink::device<devices::FC>().iset_callback_ = [this](double value)
    {
        std::string text = std::format("I = {:.1f} А", value);
        lbl_I_set_->setText(QString::fromStdString(text));
    };

    syslink::device<devices::FC>().uset_callback_ = [this](double value)
    {
        std::string text = std::format("U = {:.1f} В", value);
        lbl_U_set_->setText(QString::fromStdString(text));
    };

    syslink::device<devices::FC>().pset_callback_ = [this](double value)
    {
        std::string text = std::format("P = {:.1f} кВт", value);
        lbl_P_set_->setText(QString::fromStdString(text));
    };

    syslink::device<devices::FC>().power_callback_ = [this](bool powered)
    {
        lbl_powered_->setStyleSheet(powered? "background-color: green" : "");
    };
}

void frequency_converter_page::invert_error_mask_bit(std::size_t ireg, std::size_t ibit)
{
    errors_[ireg].mask.flip(ibit);
    bool value = errors_[ireg].mask.test(ibit);
    errors_[ireg].buttons[ibit]->setBgColor(value ? QColor(iw::color::green) : Qt::white);

    std::size_t err_reg = 0xA411 + ireg + 1;
    syslink::device<devices::FC>().set(err_reg, errors_[ireg].mask.to_ulong());

    bool err = false;
    for (std::size_t i = 0; i < errors_.size(); ++i)
        err |= errors_[i].mask.any();

    status_.set(1, err);
    if (err) status_.set(0, false);

    syslink::device<devices::FC>().set(0xA411, status_.to_ulong());
}

