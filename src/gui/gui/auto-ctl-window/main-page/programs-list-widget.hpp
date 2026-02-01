#pragma once

#include <QWidget>

#include <filesystem>

class QTableWidget;
class MessageBox;

class programs_list_widget final
    : public QWidget
{
    Q_OBJECT

    QTableWidget *local_list_;
    std::filesystem::path path_;

    MessageBox *question_msg_box_;

public:

    programs_list_widget(QWidget *);

public:

    QString current() const;

    void remove_selected();

signals:

    void row_changed();

private:

    void showEvent(QShowEvent *) override final;

    void on_row_changed(int);

private:

    void make_scroll(int);

    void make_load_file();
};

