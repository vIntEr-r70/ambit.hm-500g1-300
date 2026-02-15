#pragma once

#include <QStackedWidget>

class auto_ctl_page;
class editor_page;
class main_page;

class auto_ctl_window final
    : public QStackedWidget
{
    main_page *main_page_;
    auto_ctl_page *auto_ctl_page_;
    editor_page *editor_page_;

public:

    auto_ctl_window(QWidget *) noexcept;
};

