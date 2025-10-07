#pragma once

#include <InteractWidgets/InteractWidget.h>
#include <filesystem>

class QTableWidget;
class RoundButton;

class ProgramListDlg
    : public InteractWidget 
{
    Q_OBJECT

public:

    ProgramListDlg(QWidget*);

public:

    void show();

    void set_guid(int);

signals:

    void makeLoadFromLocalFile(QString);

private slots:

    void onSelectInLocal(QModelIndex);

private:

    void make_scroll(int);

    void make_load_file();

    void make_delete_file();

private:

    QTableWidget *local_list_;
    std::filesystem::path path_;

    RoundButton *btn_remove_;
};

