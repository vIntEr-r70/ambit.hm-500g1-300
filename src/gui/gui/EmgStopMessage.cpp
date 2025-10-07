#include "EmgStopMessage.h"

EmgStopMessage::EmgStopMessage(QWidget *parent)
    : MessageBox(parent, MessageBox::HeadError)
{
    set_source("Аварийный СТОП");
    set_message("Активирован\nАварийный СТОП");
    set_buttons({});
}

