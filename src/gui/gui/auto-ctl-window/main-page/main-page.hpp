#pragma once

#include <QWidget>

class programs_list_widget;
class main_page_header_widget;

class main_page final
    : public QWidget
{
    Q_OBJECT

    main_page_header_widget *header_;
    programs_list_widget *programs_list_widget_;

public:

    main_page(QWidget *);

signals:

    void goto_ctl_page(QString const &);

    void goto_editor_page(QString const &);

private:

    void make_create_program();

    void make_edit_program();

    void make_init_program();

    void make_remove_program();

private:

    void selected_program_changed();
};
