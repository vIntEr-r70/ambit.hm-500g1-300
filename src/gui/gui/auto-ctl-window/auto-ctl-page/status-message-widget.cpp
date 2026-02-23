#include "status-message-widget.hpp"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <qchar.h>

status_message_widget::status_message_widget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("status_message_widget");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#status_message_widget { border-radius: 20px; background-color: white; }");

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 0);
    effect->setColor(Qt::gray);
    effect->setBlurRadius(20);
    setGraphicsEffect(effect);

    QHBoxLayout *hL = new QHBoxLayout(this);
    {
        msg_ = new QLabel(this);
        msg_->setAlignment(Qt::AlignCenter);
        msg_->setStyleSheet("font-size: 16pt; color: #E55056;");
        hL->addWidget(msg_);
    }
}

void status_message_widget::update(eng::sibus::istatus status, std::string_view emsg)
{
    if (status != eng::sibus::istatus::blocked)
        msgs_[eng::sibus::istatus::blocked].clear();

    if (status != eng::sibus::istatus::ready || !emsg.empty())
        msgs_[status] = emsg;

    emsg = msgs_[eng::sibus::istatus::ready];
    if (emsg.empty())
        emsg = msgs_[eng::sibus::istatus::blocked];

    if (emsg.empty())
    {
        hide();
        return;
    }

    if (emsg == "target-external-block")
        emsg = "Необходимо активировать автоматический режим";

    msg_->setText(QString::fromUtf8(emsg.data(), emsg.length()));

    QWidget::show();
}

void status_message_widget::reset()
{
    msgs_[eng::sibus::istatus::ready].clear();

    std::string_view emsg = msgs_[eng::sibus::istatus::ready];
    if (emsg.empty())
        emsg = msgs_[eng::sibus::istatus::blocked];

    if (emsg.empty())
        hide();
}
