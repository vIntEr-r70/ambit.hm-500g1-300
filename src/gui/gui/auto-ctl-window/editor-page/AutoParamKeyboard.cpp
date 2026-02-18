#include "AutoParamKeyboard.h"

#include <Widgets/RoundButton.h>
#include <InteractWidgets/KeyboardWidget.h>
#include <Widgets/ValueSetBool.h>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QDoubleValidator>
#include <QStackedWidget>

AutoParamKeyboard::AutoParamKeyboard(QWidget* parent)
    : InteractWidget(parent)
{
	setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("border-radius: 20px; background-color: #c8c8c8");

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    vL->setSpacing(30);
    {
        title_ = new QLabel(this);
        title_->setStyleSheet("color: white; font: bold 20pt");
        vL->addWidget(title_);

        QVBoxLayout* vvL = new QVBoxLayout();
        vvL->setSpacing(5);
        {
            vvL->addStretch();

            stack_ = new QStackedWidget(this);
            {
                stack_->addWidget(create_main_page());
                stack_->addWidget(create_pause_page());
                stack_->addWidget(create_loop_page());
                stack_->addWidget(create_fc_page());
                stack_->addWidget(create_center_page());
            }
            vvL->addWidget(stack_);

            vvL->addStretch();
        }
        vL->addLayout(vvL);

        auto hL = new QHBoxLayout();
        {
            std::array<QPair<QString, int>, 12> const syms = {{
                { "7", Qt::Key_7 },     { "8", Qt::Key_8 }, { "9", Qt::Key_9 },
                { "4", Qt::Key_4 },     { "5", Qt::Key_5 }, { "6", Qt::Key_6 },
                { "1", Qt::Key_1 },     { "2", Qt::Key_2 }, { "3", Qt::Key_3 },
                { ".", Qt::Key_Period}, { "0", Qt::Key_0 }, { "−", Qt::Key_Minus }
            }};

            QGridLayout* gL = new QGridLayout();
            gL->setSpacing(20);
            {
                for (std::size_t i = 0; i < syms.size(); ++i)
                {
                    auto const& sym = syms[i];
                    if (sym.second == 0)
                        continue;

                    RoundButton *btn = KeyboardWidget::createKeyBtn(this, sym.second);
                    connect(btn, SIGNAL(clickedId(int)), this, SLOT(onKeyPressed(int)));
                    btn->setText(sym.first);
                    buttons_[sym.second] = btn;
                    gL->addWidget(btn, i / 3, i % 3);
                }
            }
            hL->addLayout(gL);

            auto vL = new QVBoxLayout();
            vL->setSpacing(20);
            {
                RoundButton* btn = KeyboardWidget::createKeyBtn(this);
                btn->setText("AC");
                connect(btn, SIGNAL(clicked()), this, SLOT(onACKey()));
                btn->setMinimumWidth(90);
                vL->addWidget(btn);

                btn = KeyboardWidget::createKeyBtn(this);
                connect(btn, SIGNAL(clicked()), this, SLOT(onBackspaceKey()));
                btn->setIcon(":/kbd.dlg.undo");
                btn->setMinimumWidth(90);
                vL->addWidget(btn);

                vL->addStretch();
            }
            hL->addLayout(vL);
        }
        vL->addLayout(hL);

        hL = new QHBoxLayout();
        {
            btn_accept_ = new RoundButton(this);
            connect(btn_accept_, &RoundButton::clicked, [this] 
            {
                InteractWidget::hide();
                std::invoke(on_accept_handler_, this);
            });
            btn_accept_->setBgColor(QColor("#29AC39"));
            btn_accept_->setIcon(":/check-mark");
            hL->addWidget(btn_accept_);

            auto btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { InteractWidget::hide(); });
            btn->setIcon(":/kbd.dlg.cancel");
            btn->setBgColor("#E55056");
            btn->setMinimumWidth(90);
            hL->addWidget(btn);
        }
        vL->addLayout(hL);
    }
}

void AutoParamKeyboard::show_main(QString const& title, QString const &v, double min, double max, main_accept_cb_t &&cb)
{
    main_page_.position.reset();
    show_main(title, v, std::move(cb));
}

void AutoParamKeyboard::show_main(QString const& title, QString const& v, double pos, double min, double max, main_accept_cb_t &&cb)
{
    main_page_.position = pos;
    show_main(title, v, std::move(cb));
}

void AutoParamKeyboard::show_main(QString const& title, QString const &v, main_accept_cb_t &&cb)
{
    title_->setText(title);

    main_accept_cb_ = std::move(cb);
    stack_->setCurrentWidget(main_page_.w);
    on_accept_handler_ = &AutoParamKeyboard::main_accept_handler;
    on_focus_handler_ = nullptr;
    on_acc_handler_ = &AutoParamKeyboard::main_acc_handler;

    main_page_.le->setText(v);
    main_page_.btn->setVisible(main_page_.position.has_value());

    le_.clear();
    le_.push_back(main_page_.le);

    InteractWidget::show();
}

void AutoParamKeyboard::show_pause(std::uint64_t msec, pause_accept_cb_t &&cb)
{
    title_->setText("Пауза");

    pause_accept_cb_ = std::move(cb);
    stack_->setCurrentWidget(pause_page_.w);
    on_accept_handler_ = &AutoParamKeyboard::pause_accept_handler;
    on_focus_handler_ = &AutoParamKeyboard::pause_focus_handler;
    on_acc_handler_ = nullptr;

    le_.clear();
    std::for_each(pause_page_.le, pause_page_.le + std::size(pause_page_.le), 
            [this](auto le) { le_.push_back(le); });

    auto sec = msec / 1000;
    char buf[64];

    std::snprintf(buf, sizeof(buf), "%02zu", sec / 3600);
    pause_page_.le[0]->setText(buf);

    std::snprintf(buf, sizeof(buf), "%02zu", (sec % 3600) / 60);
    pause_page_.le[1]->setText(buf);

    std::snprintf(buf, sizeof(buf), "%02zu.%01zu", sec % 60, (msec % 1000) / 100);
    pause_page_.le[2]->setText(buf);

    InteractWidget::show();
}

void AutoParamKeyboard::show_loop(std::size_t opid, std::size_t N, loop_accept_cb_t &&cb)
{
    title_->setText("Цикл");

    loop_accept_cb_ = std::move(cb);
    stack_->setCurrentWidget(loop_page_.w);
    on_accept_handler_ = &AutoParamKeyboard::loop_accept_handler;
    on_focus_handler_ = nullptr;
    on_acc_handler_ = nullptr;

    le_.clear();
    std::for_each(loop_page_.le, loop_page_.le + std::size(loop_page_.le), 
            [this](auto le) { le_.push_back(le); });

    loop_page_.le[0]->setText(QString::number(opid + 1));
    loop_page_.le[1]->setText(QString::number(N));

    InteractWidget::show();
}

void AutoParamKeyboard::show_fc(float p, float i, float tv, fc_accept_cb_t &&cb)
{
    title_->setText("Индуктор");

    fc_accept_cb_ = std::move(cb);
    stack_->setCurrentWidget(fc_page_.w);
    on_accept_handler_ = &AutoParamKeyboard::fc_accept_handler;
    on_focus_handler_ = nullptr;
    on_acc_handler_ = nullptr;

    le_.clear();
    std::for_each(fc_page_.le, fc_page_.le + std::size(fc_page_.le),
            [this](auto le) { le_.push_back(le); });

    fc_page_.le[0]->setText(QString::number(p, 'f', 2));
    fc_page_.le[1]->setText(QString::number(i, 'f', 1));
    fc_page_.le[2]->setText(QString::number(tv, 'f', 1));

    InteractWidget::show();
}

void AutoParamKeyboard::show_center(centering_type type, float shift, center_accept_cb_t &&cb)
{
    title_->setText("Позиционирование индуктора");

    center_accept_cb_ = std::move(cb);
    stack_->setCurrentWidget(center_page_.w);
    on_accept_handler_ = &AutoParamKeyboard::center_accept_handler;
    on_focus_handler_ = nullptr;
    on_acc_handler_ = nullptr;

    switch(type)
    {
    case centering_type::shaft:
        center_page_.type->set_value(true);
        break;
    case centering_type::tooth_in:
    case centering_type::tooth_out:
        center_page_.type->set_value(false);
        center_page_.tooth->set_value(type == centering_type::tooth_in);
        center_page_.le->setText(QString::number(shift, 'f', 1));
        break;
    case centering_type::not_active:
        break;
    }

    update_center_type();

    le_.clear();
    le_.push_back(center_page_.le);

    InteractWidget::show();
}

void AutoParamKeyboard::showEvent(QShowEvent*)
{
    if (on_focus_handler_)
        std::invoke(on_focus_handler_, this);
    else
    {
        le_.front()->setFocus();
        le_.front()->selectAll();
    }
}

void AutoParamKeyboard::update_accept_button(bool allow)
{
    btn_accept_->setEnabled(allow);
}

void AutoParamKeyboard::on_le_changed()
{
    auto it = std::find_if_not(le_.begin(), le_.end(), [](auto le) {
        return le->hasAcceptableInput();
    });
    update_accept_button(it == le_.end());
}

void AutoParamKeyboard::onKeyPressed(int key)
{
    auto it = std::find_if(le_.begin(), le_.end(), [this](auto le) { 
        return le == QWidget::focusWidget(); 
    });
    if (it == le_.end()) it = le_.begin();

    KeyboardWidget::sendKeyPressed(*it, key, buttons_[key]->text());
}

void AutoParamKeyboard::onACKey()
{
    if (on_acc_handler_ == nullptr)
    {
        auto it = std::find_if(le_.begin(), le_.end(), [this](auto le) { 
            return le == QWidget::focusWidget(); 
        });
        if (it == le_.end()) it = le_.begin();

        (*it)->setText("0");
        (*it)->selectAll();

        return;
    }

    std::invoke(on_acc_handler_, this);
}

void AutoParamKeyboard::onBackspaceKey()
{
    auto it = std::find_if(le_.begin(), le_.end(), [this](auto le) { 
        return le == QWidget::focusWidget(); 
    });
    if (it == le_.end()) it = le_.begin();

    KeyboardWidget::sendKeyPressed(*it, Qt::Key_Backspace);
}

QWidget* AutoParamKeyboard::create_main_page()
{
    QWidget *w = new QWidget();
    main_page_.w = w;

    main_page_.validator = new QDoubleValidator(w);
    main_page_.validator->setDecimals(6);

    {
        QHBoxLayout *hL = new QHBoxLayout(w);
        {
            main_page_.le = create_line_edit(w);
            main_page_.le->setValidator(main_page_.validator);
            hL->addWidget(main_page_.le);

            main_page_.btn = new RoundButton(w);
            main_page_.btn->setFixedWidth(50);
            connect(main_page_.btn, &RoundButton::clicked, [this]
            {
                double pos = main_page_.position ? *main_page_.position : 0.0;
                main_page_.le->setText(QString::number(pos, 'f', 4));
            });
            main_page_.btn->setIcon(":/cnc.position");
            main_page_.btn->setBgColor(QColor("#29AC39"));
            hL->addWidget(main_page_.btn);
        }
    }
    return w;
}

QWidget* AutoParamKeyboard::create_pause_page()
{
    QWidget *w = new QWidget();
    pause_page_.w = w;

    QVBoxLayout *vL = new QVBoxLayout(w);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            QStringList ttl{ "ЧАС", "МИН", "СЕК" };
            for (std::size_t i = 0; i < 3; ++i)
            {
                QLabel* lbl = new QLabel(ttl[i], w);
                lbl->setStyleSheet("color: white;");
                lbl->setAlignment(Qt::AlignCenter);
                hL->addWidget(lbl);
            }
        }
        vL->addLayout(hL);

        hL = new QHBoxLayout();
        for (std::size_t i = 0; i < 3; ++i)
        {
            pause_page_.le[i] = create_line_edit(w);
            pause_page_.le[i]->setMaximumWidth(90);
            pause_page_.validator[i] = new QDoubleValidator(w);
            pause_page_.le[i]->setValidator(pause_page_.validator[i]);
            hL->addWidget(pause_page_.le[i]);
        }
        vL->addLayout(hL);
    }

    return w;
}

QWidget* AutoParamKeyboard::create_fc_page()
{
    QWidget *w = new QWidget();
    fc_page_.w = w;

    QVBoxLayout *vL = new QVBoxLayout(w);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            QStringList ttl{ "кВт", "А", "сек" };
            for (std::size_t i = 0; i < 3; ++i)
            {
                QLabel* lbl = new QLabel(ttl[i], w);
                lbl->setStyleSheet("color: white;");
                lbl->setAlignment(Qt::AlignCenter);
                hL->addWidget(lbl);
            }
        }
        vL->addLayout(hL);

        hL = new QHBoxLayout();
        for (std::size_t i = 0; i < 3; ++i)
        {
            fc_page_.le[i] = create_line_edit(w);
            fc_page_.le[i]->setMaximumWidth(i == 2 ? 110 : 90);
            fc_page_.validator[i] = new QDoubleValidator(w);
            fc_page_.le[i]->setValidator(fc_page_.validator[i]);
            hL->addWidget(fc_page_.le[i]);
        }
        vL->addLayout(hL);
    }

    return w;
}

QWidget* AutoParamKeyboard::create_loop_page()
{
    QWidget *w = new QWidget();
    loop_page_.w = w;

    QVBoxLayout *vL = new QVBoxLayout(w);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            QStringList ttl{ "Переход на", "Повторов" };
            for (std::size_t i = 0; i < 2; ++i)
            {
                QLabel* lbl = new QLabel(ttl[i], w);
                lbl->setStyleSheet("color: white;");
                lbl->setAlignment(Qt::AlignCenter);
                hL->addWidget(lbl);
            }
        }
        vL->addLayout(hL);

        hL = new QHBoxLayout();
        for (std::size_t i = 0; i < 2; ++i)
        {
            loop_page_.le[i] = create_line_edit(w);
            loop_page_.le[i]->setMaximumWidth(130);
            loop_page_.validator[i] = new QDoubleValidator(w);
            loop_page_.le[i]->setValidator(loop_page_.validator[i]);
            hL->addWidget(loop_page_.le[i]);
        }
        vL->addLayout(hL);
    }

    return w;
}

QWidget* AutoParamKeyboard::create_center_page()
{
    QWidget *w = new QWidget();
    center_page_.w = w;

    QVBoxLayout *vL = new QVBoxLayout(w);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            ValueSetBool *vsb = new ValueSetBool(w);
            connect(vsb, &ValueSetBool::onValueChanged, [this] { update_center_type(); });
            vsb->setValueView("Зуб", "Вал");
            vsb->setMinimumWidth(80);
            vsb->setMaximumWidth(80);
            vsb->setTitle("Деталь");
            center_page_.type = vsb;
            hL->addWidget(vsb);

            vsb = new ValueSetBool(w);
            vsb->setValueView("Внешний", "Внутренний");
            vsb->setTitle("Тип зуба");
            vsb->setMinimumWidth(100);
            center_page_.tooth = vsb;
            hL->addWidget(vsb);

            center_page_.le = create_line_edit(w);
            center_page_.le->setMaximumWidth(90);
            hL->addWidget(center_page_.le);

            hL->setAlignment(center_page_.type, Qt::AlignLeft);
        }
        vL->addLayout(hL);
    }

    return w;
}

void AutoParamKeyboard::update_center_type() noexcept
{
    bool v = center_page_.type->value();
    center_page_.tooth->setVisible(!v);
    center_page_.le->setVisible(!v);
}

QLineEdit* AutoParamKeyboard::create_line_edit(QWidget* w)
{
    QLineEdit *le = new QLineEdit(w);
    connect(le, &QLineEdit::textChanged, [this] (QString const&) { on_le_changed(); });
    le->setStyleSheet("background-color: white;"
                  "border-radius: 15px;"
                  "font: bold 20pt;"
                  "padding: 10px;");
    le->setAlignment(Qt::AlignCenter);
    return le;
}

void AutoParamKeyboard::main_accept_handler()
{
    float cv = main_page_.le->text().toFloat();
    main_accept_cb_(cv);
}

void AutoParamKeyboard::main_acc_handler()
{
    main_page_.le->setText("0");
    main_page_.le->selectAll();
}

void AutoParamKeyboard::pause_accept_handler()
{
    std::uint64_t msec = pause_page_.le[0]->text().toUInt() * 3600000;
    msec += pause_page_.le[1]->text().toUInt() * 60000;
    msec += pause_page_.le[2]->text().toFloat() * 1000;

    pause_accept_cb_(msec); 
}

void AutoParamKeyboard::pause_focus_handler()
{
    pause_page_.le[2]->setFocus();
    pause_page_.le[2]->selectAll();
}

void AutoParamKeyboard::loop_accept_handler()
{
    std::size_t opid = loop_page_.le[0]->text().toUInt() - 1;
    std::size_t N = loop_page_.le[1]->text().toUInt();

    loop_accept_cb_(opid, N); 
}

void AutoParamKeyboard::fc_accept_handler()
{
    float p = fc_page_.le[0]->text().toFloat();
    float i = fc_page_.le[1]->text().toFloat();
    float tv = fc_page_.le[2]->text().toFloat();

    fc_accept_cb_(p, i, tv);
}

void AutoParamKeyboard::center_accept_handler()
{
    centering_type type = centering_type::shaft;
    if (!center_page_.type->value())
        type = center_page_.tooth->value() ? centering_type::tooth_in : centering_type::tooth_out;

    float shift = center_page_.le->text().toFloat();

    center_accept_cb_(type, shift); 
}
