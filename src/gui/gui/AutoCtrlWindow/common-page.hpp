#pragma once

#include <QWidget>

class programs_list_widget;
class common_page_header_widget;

class common_page final
    : public QWidget
{
    Q_OBJECT

    common_page_header_widget *header_;
    programs_list_widget *programs_list_widget_;

public:

    common_page(QWidget *);

signals:

    void goto_ctl_page(QString const &);

    void goto_editor_page(QString const &);

    // void program_changed(QString const &);

private:

    void make_create_program();

    void make_edit_program();

    void make_init_program();

    void make_remove_program();

private:

    void selected_program_changed();
};
