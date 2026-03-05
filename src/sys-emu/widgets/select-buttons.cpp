#include "select-buttons.hpp"

#include <Widgets/RoundButton.h>
#include <defines.hpp>

#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QTimer>

select_buttons::select_buttons(QWidget *parent, QString key, QString title, std::initializer_list<std::string_view> list)
    : QWidget(parent)
    , key_(key)
{
    QFont f(font());
    f.setPointSize(11);
    setFont(f);

    buttons_.reserve(list.size());

    QVBoxLayout *vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    {
        QLabel *lbl = new QLabel(this);
        lbl->setText(title);
        lbl->setAlignment(Qt::AlignCenter);
        vL->addWidget(lbl);

        QHBoxLayout *hL = new QHBoxLayout();
        {
            std::ranges::for_each(list, [this, hL](std::string_view title)
            {
                RoundButton *btn = new RoundButton(this);
                buttons_.push_back(btn);
                btn->setText(QString::fromUtf8(title.data(), title.length()));
                connect(btn, &RoundButton::clicked, [this, btn]
                {
                    switch_selection(btn);
                    save_state();
                });
                hL->addWidget(btn);
            });
        }
        vL->addLayout(hL);
    }

    std::string def_value(*list.begin());
    QTimer::singleShot(0, [this, def_value]
    {
        load_state(QString::fromStdString(def_value));
    });
}

void select_buttons::switch_selection(RoundButton *btn)
{
    if (selected_)
        selected_->setBgColor(Qt::white);

    selected_ = btn;
    selected_->setBgColor(iw::color::green);

    emit selected(btn->text());
}

static RoundButton* find_button(QString const& value, auto &list)
{
    auto it = std::ranges::find_if(list, [&value](auto btn) {
        return btn->text() == value;
    });
    return (it != list.end()) ? *it : nullptr;
}

void select_buttons::load_state(QString const &value)
{
    QSettings settings("sys-emu");
    QString v = settings.value(key_, value).toString();

    RoundButton *btn = find_button(v, buttons_);
    if (btn == nullptr)
        return;

    switch_selection(btn);
}

void select_buttons::save_state()
{
    QSettings settings("sys-emu");
    settings.setValue(key_, selected_->text());
}
