#pragma once

#include <QWidget>

class ValueViewString;
class RoundButton;

class main_page_header_widget final
    : public QWidget
{
    Q_OBJECT

    ValueViewString *name_;
    ValueViewString *comments_;

    RoundButton *btn_edit_;
    RoundButton *btn_mode_;
    RoundButton *btn_remove_;

public:

    main_page_header_widget(QWidget*) noexcept;

public:

    void set_program_info(QString, QString);

signals:

    void make_create_program();

    void make_edit_program();

    void make_init_program();

    void make_remove_program();
};
