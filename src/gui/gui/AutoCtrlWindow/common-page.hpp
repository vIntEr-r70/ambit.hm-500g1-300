#pragma once

#include <QWidget>

class programs_list_widget;

class common_page final
    : public QWidget
{
    Q_OBJECT

    programs_list_widget *programs_list_widget_;

public:

    common_page(QWidget *);

signals:

    void goto_ctl_page(QString);

    void goto_editor_page();

private:

    void load_selected_item();

    void open_selected_item();
};
