#pragma once

#include <QWidget>

class common_page final
    : public QWidget
{
    Q_OBJECT

public:

    common_page(QWidget *);

signals:

    void goto_ctl_page();

    void goto_editor_page();

private:

    void load_selected_item();

    void open_selected_item();
};
