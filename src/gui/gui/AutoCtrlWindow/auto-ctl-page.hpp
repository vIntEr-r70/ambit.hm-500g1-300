#pragma once

#include <QWidget>

#include <eng/sibus/node.hpp>

class RoundButton;
// class IconButton;
// class ProgramListDlg;
class ProgramModel;
class program_widget;
// class QTableView;
// class QTableWidget;
// class ValueViewString;
// class ValueViewTime;

class auto_ctl_page final
    : public QWidget
    , public eng::sibus::node
{
    Q_OBJECT

    ProgramModel &model_;
    program_widget *program_widget_;

    eng::sibus::output_wire_id_t ctl_;

    RoundButton *btn_start_;
    RoundButton *btn_stop_;
    RoundButton *btn_continue_;

    std::optional<std::size_t> phase_id_;
    std::size_t ctl_mode_{ 0 };

public:

    auto_ctl_page(QWidget*, ProgramModel &) noexcept;

public:

    void init(QString const &);

private:

    void make_start();

    void make_stop();

private:

    void update_widget_view();

    void update_ctl_state(eng::abc::pack);

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

// private:
//
//     void update_ctl_buttons();
//
signals:

    void make_done();
//
//     void make_start();
//
//     void make_stop();
//
//     void load_program();
//
// private slots:
//
//     void on_load_local_program(QString);

// private:

    // ValueViewString *name_;
    // ValueViewString *comments_;
    //
    // IconButton *btn_load_;
    // IconButton *btn_edit_;
    // IconButton *btn_new_;
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
