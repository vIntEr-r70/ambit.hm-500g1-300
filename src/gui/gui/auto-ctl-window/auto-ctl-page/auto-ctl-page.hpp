#pragma once

#include <QWidget>

#include <eng/sibus/node.hpp>

class RoundButton;
class ProgramModel;
class QStackedWidget;

class program_widget;
class problem_list_widget;

class auto_ctl_page final
    : public QWidget
    , public eng::sibus::node
{
    Q_OBJECT

    ProgramModel &model_;

    QStackedWidget *stack_;
    program_widget *program_widget_;
    problem_list_widget *problem_list_widget_;

    eng::sibus::output_wire_id_t ctl_;
    eng::sibus::output_port_id_t program_out_;

    RoundButton *btn_start_;
    RoundButton *btn_stop_;
    RoundButton *btn_continue_;

    std::optional<std::size_t> phase_id_;
    bool execution_error_;

    std::string program_base64_;

public:

    auto_ctl_page(QWidget*, ProgramModel &) noexcept;

public:

    void init();

private:

    void make_start();

    void make_stop();

private:

    void go_to_editor();

    void go_to_main();

    void update_widget_view();

    void update_phase_id(eng::abc::pack);

signals:

    void make_done();

    void make_edit();
};
