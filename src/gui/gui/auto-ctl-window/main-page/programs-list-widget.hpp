#pragma once

#include <QWidget>

class QTableView;
class MessageBox;
class program_list_model;
class RoundButton;
class main_page_header_widget;
class QLabel;

struct program_record_t;

class programs_list_widget final
    : public QWidget
{
    main_page_header_widget *header_;

    program_list_model *model_;
    QTableView *list_;

    MessageBox *question_msg_box_;

    RoundButton *btn_to_hd_;
    RoundButton *btn_to_usb_;

public:

    programs_list_widget(QWidget *, main_page_header_widget *);

public:

    void remove_selected();

    program_record_t const* selected_program() const;

    program_record_t const *find_program_by_name(std::string const &) const;

private:

    void showEvent(QShowEvent *) override final;

    void on_row_changed(int);

private:

    void make_scroll(int);

    void make_load_file();

    void update_selection();

    void copy_to_hd();

    void copy_to_usb();
};

