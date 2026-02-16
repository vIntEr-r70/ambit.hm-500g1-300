#pragma once

#include <QWidget>

#include <eng/sibus/node.hpp>

class RoundButton;
class QStackedWidget;
class QLabel;
class program_model_mode;
class program_widget;
class problem_list_widget;

struct program_record_t;

class auto_ctl_page final
    : public QWidget
    , public eng::sibus::node
{
    Q_OBJECT

    program_model_mode *model_;

    QStackedWidget *stack_;
    program_widget *program_widget_;
    problem_list_widget *problem_list_widget_;

    eng::sibus::output_wire_id_t ctl_;
    eng::sibus::output_port_id_t program_;

    RoundButton *btn_start_;
    RoundButton *btn_stop_;
    RoundButton *btn_continue_;

    QLabel *lbl_common_time_;
    QLabel *lbl_pause_time_;
    QLabel *lbl_program_name_;

    std::string fname_;

public:

    auto_ctl_page(QWidget*) noexcept;

public:

    void init(program_record_t const *);

private:

    void make_start();

    void make_continue();

    void make_stop();

private:

    void go_to_editor();

    void go_to_main();

    void update_widget_view();

    void update_phase_id(eng::abc::pack);

    void update_times(eng::abc::pack);

signals:

    void make_done();

    void make_edit(std::string const &);
};

