#pragma once

#include <QStackedWidget>

class ProgramModel;
class MessageBox;

class auto_ctl_page;
class editor_page;
class main_page;

class auto_ctl_window final
    : public QStackedWidget
{
    ProgramModel *model_;

    main_page *main_page_;
    auto_ctl_page *auto_ctl_page_;
    editor_page *editor_page_;

    MessageBox *msg_;

public:

    auto_ctl_window(QWidget *) noexcept;

private:

    bool load_program_to_model(QString const &);
};

