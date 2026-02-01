#pragma once

#include "common/internal-state-ctl.hpp"

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
    , public internal_state_ctl<auto_ctl_page>
{
    Q_OBJECT

    ProgramModel &model_;

    QStackedWidget *stack_;
    program_widget *program_widget_;
    problem_list_widget *problem_list_widget_;

    eng::sibus::output_wire_id_t ctl_;

    RoundButton *btn_start_;
    RoundButton *btn_stop_;
    RoundButton *btn_continue_;

    std::optional<std::size_t> phase_id_;
    bool execution_error_;

    std::size_t ctl_mode_{ 0 };

public:

    auto_ctl_page(QWidget*, ProgramModel &) noexcept;

public:

    void s_initialize();

    void s_not_available();

    void s_program_loaded();

    void s_program_load_failed();

public:

    bool init();

private:

    void make_start();

    void make_stop();

private:

    void go_to_editor();

    void update_widget_view();

    void update_phase_id(eng::abc::pack);

signals:

    void make_done();

    void make_edit();
};
