#pragma once

#include <QWidget>

// class RoundButton;
// class IconButton;
// // class ProgramListDlg;
// class ProgramModel;
// class QTableView;
class ValueViewString;
// class ValueViewTime;

class common_page_header_widget final
    : public QWidget
{
    Q_OBJECT

public:

    common_page_header_widget(QWidget*) noexcept;

public:

    // void init();
    //
    // void set_current_phase(std::size_t) noexcept;
    //
    // void set_time_front(std::size_t) noexcept;
    //
    // void set_time_back(std::size_t) noexcept;
    //
    // void set_time_pause(std::size_t) noexcept;
    //
    // void set_sys_mode(unsigned char) noexcept;
    //
    // void set_running(bool) noexcept;
    //
    // void set_pause(bool) noexcept;
    //
    // void set_guid(int);

private:

    // void update_ctl_buttons();

signals:

    void make_load();

    void make_open();

    // void make_start();
    //
    // void make_stop();
    //
    // void load_program();

private slots:

    // void on_load_local_program(QString);

private:

    // ProgramModel &model_;

    ValueViewString *name_;
    ValueViewString *comments_;
    //
    // IconButton *btn_load_;
    // IconButton *btn_edit_;
    // IconButton *btn_new_;
    //
    // RoundButton *btn_start_;
    // RoundButton *btn_stop_;
    //
    // // ProgramListDlg *list_dlg_;
    //
    // ValueViewTime *time_front_;
    // ValueViewTime *time_back_;
    // ValueViewTime *time_pause_;
    //
    // bool mode_allow_{ false };
    // bool in_pause_{ false };
    // bool running_{ false };
};
