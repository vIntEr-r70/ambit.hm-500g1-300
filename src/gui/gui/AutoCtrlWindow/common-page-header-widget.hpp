#pragma once

#include <QWidget>

#include <filesystem>

class ValueViewString;

class common_page_header_widget final
    : public QWidget
{
    Q_OBJECT

    ValueViewString *name_;
    ValueViewString *comments_;

    std::filesystem::path path_;

public:

    common_page_header_widget(QWidget*) noexcept;

public:

    void set_program_name(QString);

signals:

    void make_create_program();

    void make_edit_program();

    void make_init_program();

    void make_remove_program();
};
