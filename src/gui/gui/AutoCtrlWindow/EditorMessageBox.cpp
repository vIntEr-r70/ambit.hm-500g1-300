#include "EditorMessageBox.h"

#include <aem/log.h>

EditorMessageBox::EditorMessageBox(QWidget *parent)
    : MessageBox(parent, MessageBox::HeadError)
{
    set_source("Редактор...");
}

void EditorMessageBox::show_error(QString const &msg)
{
    set_message(msg);
    set_button("ОK");
    show([this](std::size_t) { MessageBox::hide(); });
}

void EditorMessageBox::show_question(QString const &msg, question_cb_t &&cb)
{
    set_message(msg);
    set_buttons({ "ДА", "НЕТ" });

    show([this, icb = std::move(cb)](std::size_t id)
    {
        MessageBox::hide();
        if (id == 0) icb();
    });
}

