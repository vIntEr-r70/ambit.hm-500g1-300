#include "main-page-header-widget.hpp"

#include <Widgets/ValueViewString.h>
#include <Widgets/RoundButton.h>

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>

main_page_header_widget::main_page_header_widget(QWidget *parent) noexcept
    : QWidget(parent)
{
    setObjectName("common_page_header_widget");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#common_page_header_widget { border-radius: 20px; background-color: white; }");

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 0);
    effect->setColor(Qt::gray);
    effect->setBlurRadius(20);
    setGraphicsEffect(effect);

    QVBoxLayout *vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            name_ = new ValueViewString(this);
            name_->setTitle("Название программы");
            name_->setMaximumWidth(400);
            name_->setMinimumWidth(400);
            hL->addWidget(name_);

            hL->addStretch();

            RoundButton *btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_init_program(); });
            btn->setBgColor("#29AC39");
            btn->setIcon(":/cnc.play");
            hL->addWidget(btn);
            btn_mode_ = btn;

            hL->addSpacing(20);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_create_program(); });
            btn->setBgColor("#29AC39");
            btn->setIcon(":/file.new");
            hL->addWidget(btn);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_edit_program(); });
            btn->setBgColor("#29AC39");
            btn->setIcon(":/file.edit");
            hL->addWidget(btn);
            btn_edit_ = btn;

            hL->addSpacing(20);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_remove_program(); });
            btn->setBgColor("#E55056");
            btn->setIcon(":/file.delete");
            hL->addWidget(btn);
            btn_remove_ = btn;
        }
        vL->addLayout(hL);

        comments_ = new ValueViewString(this);
        comments_->setValueAlignment(Qt::AlignLeft);
        comments_->setTitle("Коментарии");
        vL->addWidget(comments_);
    }

    set_program_info("", "");
}

void main_page_header_widget::set_program_info(QString name, QString comments)
{
    name_->set_value(name);
    comments_->set_value(comments);

    btn_edit_->setEnabled(!name.isEmpty());
    btn_mode_->setEnabled(!name.isEmpty());
    btn_remove_->setEnabled(!name.isEmpty());
}

