#pragma once

#include <QWidget>

#include <filesystem>

class QTableWidget;
// class RoundButton;

class programs_list_widget final
    : public QWidget
{
    Q_OBJECT

    QTableWidget *local_list_;
    std::filesystem::path path_;

public:

    programs_list_widget(QWidget *);

public:

    QString current() const;

private:

    void showEvent(QShowEvent *) override final;

// public:
//
//     void show();
//
//     void set_guid(int);

// signals:
//
//     void makeLoadFromLocalFile(QString);

private slots:

    void onSelectInLocal(QModelIndex);

private:

    void make_scroll(int);

    void make_load_file();

//     void make_delete_file();

// private:

//
//     RoundButton *btn_remove_;
};

