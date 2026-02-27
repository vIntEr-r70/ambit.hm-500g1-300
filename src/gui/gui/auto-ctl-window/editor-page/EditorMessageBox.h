#pragma once

#include <InteractWidgets/message-box.hpp>

class EditorMessageBox final
    : public iw::message_box
{
    typedef std::function<void()> question_cb_t;

public:

    EditorMessageBox(QWidget*);

public:

    void show_error(QString const&);

    void show_question(QString const&, question_cb_t &&);
};

