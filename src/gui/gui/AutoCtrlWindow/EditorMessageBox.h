#pragma once

#include <InteractWidgets/MessageBox.h>

class EditorMessageBox final
    : public MessageBox
{
    typedef std::function<void()> question_cb_t;

public:

    EditorMessageBox(QWidget*);

public:

    void show_error(QString const&);

    void show_question(QString const&, question_cb_t &&);
};

