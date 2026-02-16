#pragma once

#include <QWidget>

class programs_list_widget;
class main_page_header_widget;
struct program_record_t;

class main_page final
    : public QWidget
{
    Q_OBJECT

    main_page_header_widget *header_;
    programs_list_widget *programs_list_widget_;

public:

    main_page(QWidget *);

public:

    program_record_t const *find_program_by_name(std::string const &) const;

signals:

    void goto_ctl_page(program_record_t const *);

    void goto_editor_page(program_record_t const *);

private:

    void make_create_program();

    void make_edit_program();

    void make_init_program();
};
