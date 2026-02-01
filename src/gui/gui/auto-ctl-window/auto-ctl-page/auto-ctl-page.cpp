#include "auto-ctl-page.hpp"
#include "problem-list-widget.hpp"

#include <Widgets/ValueViewString.h>
#include <Widgets/ValueViewTime.h>
#include <Widgets/RoundButton.h>
#include <Widgets/IconButton.h>
#include <Widgets/VerticalScroll.h>

#include "../common/ProgramModel.h"
#include "../common/program-widget.hpp"

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QTableWidget>
#include <QStackedWidget>

#include <eng/log.hpp>
#include <qstackedwidget.h>

auto_ctl_page::auto_ctl_page(QWidget *parent, ProgramModel &model) noexcept
    : QWidget(parent)
    , eng::sibus::node("auto-gui-ctl")
    , model_(model)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        QWidget *w = new QWidget(this);
        {
            w->setObjectName("auto_ctl_page");
            w->setAttribute(Qt::WA_StyledBackground, true);
            w->setStyleSheet("QWidget#auto_ctl_page { border-radius: 20px; background-color: white; }");

            QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(w);
            effect->setOffset(0, 0);
            effect->setColor(Qt::gray);
            effect->setBlurRadius(20);
            w->setGraphicsEffect(effect);

            QHBoxLayout *hL = new QHBoxLayout(w);
            {
                IconButton *btn_exit = new IconButton(this, ":/check-mark");
                connect(btn_exit, &IconButton::clicked, [this] { emit make_done(); });
                btn_exit->setBgColor("#8a8a8a");
                hL->addWidget(btn_exit);

                IconButton *btn_edit = new IconButton(this, ":/EditFile");
                connect(btn_edit, &IconButton::clicked, [this] { go_to_editor(); });
                btn_edit->setBgColor("#8a8a8a");
                hL->addWidget(btn_edit);

                RoundButton *btn = new RoundButton(w);
                connect(btn, &RoundButton::clicked, [this] { make_start(); });
                btn->setText("СТАРТ");
                btn->setBgColor("#29AC39");
                btn->setMinimumWidth(150);
                btn_start_ = btn;
                hL->addWidget(btn);

                btn = new RoundButton(w);
                connect(btn, &RoundButton::clicked, [this] { make_start(); });
                btn->setText("ДАЛЕЕ");
                btn->setBgColor("#29AC39");
                btn->setMinimumWidth(150);
                btn_continue_ = btn;
                hL->addWidget(btn);

                btn = new RoundButton(w);
                connect(btn, &RoundButton::clicked, [this] { make_stop(); });
                btn->setText("СТОП");
                btn->setBgColor("#E55056");
                btn->setMinimumWidth(100);
                btn_stop_ = btn;
                hL->addWidget(btn);
            }
        }
        vL->addWidget(w);

        stack_ = new QStackedWidget(this);
        {
            program_widget_ = new program_widget(this, model);
            stack_->addWidget(program_widget_);

            problem_list_widget_ = new problem_list_widget(this);
            stack_->addWidget(problem_list_widget_);
        }
        vL->addWidget(stack_);
    }

    node::add_input_port_v2("phase-id", [this](eng::abc::pack args)
    {
        update_phase_id(std::move(args));
    });

    ctl_ = node::add_output_wire();
    node::set_wire_response_handler(ctl_, [this](bool success, eng::abc::pack)
    {
        if (success) switch_to_state(&auto_ctl_page::s_program_loaded);
        else switch_to_state(&auto_ctl_page::s_program_load_failed);
    });

    node::set_wire_status_handler(ctl_, [this] {
        update_widget_view();
    });

    update_widget_view();
}

// Инициализируем модуль выполнения автоматического режима
void auto_ctl_page::s_initialize()
{
    stack_->setCurrentWidget(program_widget_);

    // Проверяем доступность узла
    if (!node::is_ready(ctl_))
    {
        problem_list_widget_->set_problem(problem_flag::module_access, true);
        switch_to_state(&auto_ctl_page::s_not_available);
        return;
    }
    problem_list_widget_->set_problem(problem_flag::module_access, false);

    // Передаем программу в узел автоматического режима
    std::string base64 = model_.get_base64_program();
    node::send_wire_signal(ctl_, { base64 });
}

void auto_ctl_page::s_program_loaded()
{
    problem_list_widget_->set_problem(problem_flag::programm, false);
    switch_to_state(nullptr);
}

void auto_ctl_page::s_program_load_failed()
{
    problem_list_widget_->set_problem(problem_flag::programm, true);
    stack_->setCurrentWidget(problem_list_widget_);
    switch_to_state(nullptr);
}

void auto_ctl_page::s_not_available()
{
    stack_->setCurrentWidget(problem_list_widget_);
    switch_to_state(nullptr);
}

void auto_ctl_page::go_to_editor()
{
    model_.clear_current_row();
    emit make_edit();
}

bool auto_ctl_page::init()
{
    if (is_in_state())
        return false;
    switch_to_state(&auto_ctl_page::s_initialize);
    return true;
}

void auto_ctl_page::make_start()
{
    // btn_start_->setEnabled(false);
    // node::activate(ctl_, { });
}

void auto_ctl_page::make_stop()
{
    // node::deactivate(ctl_);
}

void auto_ctl_page::update_widget_view()
{
    // btn_stop_->hide();
    // btn_start_->hide();
    // btn_continue_->hide();
    //
    // if (node::is_ready(ctl_) && !node::is_transiting(ctl_))
    // {
    //     btn_start_->show();
    //     btn_start_->setEnabled(true);
    // }
    // else if (node::is_active(ctl_))
    // {
    //     btn_stop_->show();
    // }
    // else
    // {
    //     btn_start_->show();
    //     btn_start_->setEnabled(false);
    // }

    //
    // if ( && )
    // {
    //     switch(ctl_mode_)
    //     {
    //     case 0: // idle
    //         break;
    //     case 1: // run
    //         break;
    //     case 2: // pause
    //         btn_stop_->hide();
    //         btn_start_->hide();
    //         btn_continue_->show();
    //         break;
    //     }
    // }
    // else
    // {
    //
    //     btn_stop_->hide();
    //     btn_continue_->hide();
    // }
}

void auto_ctl_page::update_phase_id(eng::abc::pack args)
{
    // if (!args)
    // {
    //     eng::log::info("{}: phase-id = {}", name(), "NULL");
    //     phase_id_.reset();
    // }
    // else
    // {
    //     phase_id_ = eng::abc::get<std::uint32_t>(args, 0);
    //     eng::log::info("{}: phase-id = {}", name(), phase_id_.value());
    //
    //     execution_error_ = eng::abc::get<bool>(args, 1);
    //     // ctl_mode_ = eng::abc::get<std::uint32_t>(args, 1);
    //     model_.set_current_phase(*phase_id_);
    // }
    //
    // update_widget_view();
}
