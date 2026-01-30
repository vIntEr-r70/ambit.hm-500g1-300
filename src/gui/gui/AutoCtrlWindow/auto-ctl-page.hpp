#pragma once

#include <QWidget>

#include <eng/sibus/node.hpp>

class RoundButton;
class ProgramModel;
class program_widget;

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
    bool execution_error_;

    std::size_t ctl_mode_{ 0 };

public:

    auto_ctl_page(QWidget*, ProgramModel &) noexcept;

public:

    void init(QString const &);

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
