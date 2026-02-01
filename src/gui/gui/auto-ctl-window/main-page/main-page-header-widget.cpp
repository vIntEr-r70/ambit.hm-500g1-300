#include "main-page-header-widget.hpp"

#include "../common/ProgramModel.h"

#include <Widgets/ValueViewString.h>
#include <Widgets/RoundButton.h>

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>

main_page_header_widget::main_page_header_widget(QWidget *parent) noexcept
    : QWidget(parent)
{
    auto LIAEM_RW_PATH = std::getenv("LIAEM_RW_PATH");
    path_ = LIAEM_RW_PATH ? LIAEM_RW_PATH : ".";
    path_ /= "programs";

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
            connect(btn, &RoundButton::clicked, [this] { emit make_create_program(); });
            btn->setText("Создать");
            btn->setBgColor("#29AC39");
            btn->setTextColor(Qt::white);
            btn->setMinimumWidth(100);
            hL->addWidget(btn);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_edit_program(); });
            btn->setText("Изменить");
            btn->setBgColor("#29AC39");
            btn->setTextColor(Qt::white);
            btn->setMinimumWidth(100);
            hL->addWidget(btn);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_init_program(); });
            btn->setText("Загрузить");
            btn->setBgColor("#29AC39");
            btn->setTextColor(Qt::white);
            btn->setMinimumWidth(100);
            hL->addWidget(btn);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_remove_program(); });
            btn->setText("Удалить");
            btn->setBgColor("#E55056");
            btn->setTextColor(Qt::white);
            btn->setMinimumWidth(100);
            hL->addWidget(btn);
        }
        vL->addLayout(hL);

        comments_ = new ValueViewString(this);
        comments_->setValueAlignment(Qt::AlignLeft);
        comments_->setTitle("Коментарии");
        vL->addWidget(comments_);
    }
}

void main_page_header_widget::set_program_name(QString name)
{
    name_->set_value(name);

    if (name.isEmpty())
    {
        comments_->set_value("");
        return;
    }

    QString fname{ QString::fromLocal8Bit(
        (path_ / name.toUtf8().constData()).c_str()) };

    QString comments;
    if (ProgramModel::load_program_comments(fname, comments))
        comments_->set_value(comments);
    else
        comments_->set_value("");
}


