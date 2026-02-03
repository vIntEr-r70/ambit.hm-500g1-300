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
                connect(btn_exit, &IconButton::clicked, [this] { go_to_main(); });
                btn_exit->setBgColor("#8a8a8a");
                hL->addWidget(btn_exit);

                IconButton *btn_edit = new IconButton(this, ":/EditFile");
                connect(btn_edit, &IconButton::clicked, [this] { go_to_editor(); });
                btn_edit->setBgColor("#8a8a8a");
                hL->addWidget(btn_edit);

                hL->addStretch();

                RoundButton *btn = new RoundButton(w);
                connect(btn, &RoundButton::clicked, [this] { make_start(); });
                btn->setText("СТАРТ");
                btn->setBgColor("#29AC39");
                btn->setFixedWidth(150);
                btn_start_ = btn;
                btn->hide();
                hL->addWidget(btn);

                btn = new RoundButton(w);
                connect(btn, &RoundButton::clicked, [this] { make_start(); });
                btn->setText("ДАЛЕЕ");
                btn->setBgColor("#29AC39");
                btn->setFixedWidth(150);
                btn_continue_ = btn;
                btn->hide();
                hL->addWidget(btn);

                btn = new RoundButton(w);
                connect(btn, &RoundButton::clicked, [this] { make_stop(); });
                btn->setText("СТОП");
                btn->setBgColor("#E55056");
                btn->setFixedWidth(150);
                btn_stop_ = btn;
                btn->hide();
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

    node::add_input_port_v2("phase-id", [this](eng::abc::pack args) {
        update_phase_id(std::move(args));
    });

    ctl_ = node::add_output_wire();
    node::set_wire_status_handler(ctl_, [this] {
        update_widget_view();
    });

    program_out_ = node::add_output_port("program");

    update_widget_view();
}

// // Инициализируем модуль выполнения автоматического режима
// void auto_ctl_page::s_initialize()
// {
//     eng::log::info("{}: {}", name(), __func__);
//
//     // Проверяем доступность узла
//     if (node::is_blocked(ctl_))
//     {
//         stack_->setCurrentWidget(problem_list_widget_);
//         switch_to_state(&auto_ctl_page::s_mode_was_blocked);
//         return;
//     }
//
//     stack_->setCurrentWidget(program_widget_);
//     switch_to_state(&auto_ctl_page::s_ready_to_work);
//
//     //
//     // stack_->setCurrentWidget(program_widget_);
//     //
//     // // Проверяем доступность узла
//     // if (!node::is_ready(ctl_))
//     // {
//     //     problem_list_widget_->set_problem(problem_flag::module_access, true);
//     //     switch_to_state(&auto_ctl_page::s_not_available);
//     //     return;
//     // }
//     // problem_list_widget_->set_problem(problem_flag::module_access, false);
//     //
//     // // Передаем программу в узел автоматического режима
//     // std::string base64 = model_.get_base64_program();
//     // node::send_wire_signal(ctl_, { base64 });
// }

// void auto_ctl_page::s_ready_to_work()
// {
//     eng::log::info("{}: {}", name(), __func__);
//
//     if (node::is_blocked(ctl_))
//     {
//         switch_to_state(&auto_ctl_page::s_initialize);
//         return;
//     }
//
//     switch_to_state(&auto_ctl_page::s_wait_program);
// }

// void auto_ctl_page::s_wait_program()
// {
//     eng::log::info("{}: {}", name(), __func__);
//
//     if (node::is_blocked(ctl_))
//     {
//         switch_to_state(&auto_ctl_page::s_initialize);
//         return;
//     }
//
//     if (program_base64_.empty())
//         return;
//
//     // Передаем программу в узел автоматического режима
//     node::send_wire_signal(ctl_, { program_base64_ });
//     switch_to_state(&auto_ctl_page::s_ready_to_start);
// }

// void auto_ctl_page::s_mode_was_blocked()
// {
//     eng::log::info("{}: {}", name(), __func__);
//
//     if (node::is_ready(ctl_))
//     {
//         stack_->setCurrentWidget(program_widget_);
//         switch_to_state(&auto_ctl_page::s_ready_to_work);
//         return;
//     }
//
//     // Запрашиваем причину блокировки
//     problem_list_widget_->clear();
//     node::request_wire_block_reasons(ctl_, [this](std::vector<std::string_view> reasons)
//     {
//         std::ranges::for_each(reasons, [this](auto sv)
//         {
//             problem_list_widget_->append(sv);
//         });
//     });
// }

// void auto_ctl_page::s_infinity_pause()
// {
//     eng::log::info("{}: {}", name(), __func__);
//
//     if (node::is_blocked(ctl_))
//     {
//         switch_to_state(&auto_ctl_page::s_initialize);
//         return;
//     }
//
//     btn_start_->hide();
//
//     btn_stop_->show();
//     btn_stop_->setEnabled(true);
//
//     btn_continue_->show();
//     btn_continue_->setEnabled(true);
// }

// void auto_ctl_page::s_ready_to_start()
// {
//     eng::log::info("{}: {}", name(), __func__);
//
//     if (node::is_blocked(ctl_))
//     {
//         switch_to_state(&auto_ctl_page::s_initialize);
//         return;
//     }
//
//     btn_stop_->hide();
//     btn_continue_->hide();
//
//     btn_start_->show();
//     btn_start_->setEnabled(true);
// }

// void auto_ctl_page::s_program_started()
// {
//     eng::log::info("{}: {}", name(), __func__);
//
//     btn_start_->hide();
//     btn_continue_->hide();
//
//     btn_stop_->show();
//     btn_stop_->setEnabled(true);
//
//     if (node::is_active(ctl_))
//         return;
//
//     switch_to_state(&auto_ctl_page::s_ready_to_work);
// }

// void auto_ctl_page::s_program_loaded()
// {
//     // problem_list_widget_->set_problem(problem_flag::programm, false);
//     switch_to_state(nullptr);
// }

// void auto_ctl_page::s_program_load_failed()
// {
//     // problem_list_widget_->set_problem(problem_flag::programm, true);
//     stack_->setCurrentWidget(problem_list_widget_);
//     switch_to_state(nullptr);
// }

// void auto_ctl_page::s_not_available()
// {
//     stack_->setCurrentWidget(problem_list_widget_);
//     switch_to_state(nullptr);
// }

void auto_ctl_page::go_to_editor()
{
    // Удаляем программу из модуля режима
    node::set_port_value(program_out_, { });

    model_.clear_current_row();
    emit make_edit();
}

void auto_ctl_page::go_to_main()
{
    // Удаляем программу из модуля режима
    node::set_port_value(program_out_, { });

    model_.clear_current_row();
    emit make_done();
}

void auto_ctl_page::init()
{
    // Задаем программу в модуль режима
    node::set_port_value(program_out_, { model_.get_base64_program() });
}

void auto_ctl_page::make_start()
{
    if (!node::is_ready(ctl_))
        return;

    btn_start_->setEnabled(false);
    node::activate(ctl_, { });
}

void auto_ctl_page::make_stop()
{
    if (!node::is_active(ctl_))
        return;

    btn_stop_->setEnabled(false);
    node::deactivate(ctl_);
}

void auto_ctl_page::update_widget_view()
{
    if (node::is_transiting(ctl_))
        return;

    btn_stop_->hide();
    btn_start_->hide();
    btn_continue_->hide();

    if (node::is_blocked(ctl_))
    {
        problem_list_widget_->clear();
        node::for_each_block_reasons(ctl_, [this](std::string_view emsg)
        {
            problem_list_widget_->append(emsg);
        });
        stack_->setCurrentWidget(problem_list_widget_);
    }
    else
    {
        stack_->setCurrentWidget(program_widget_);

        if (node::is_ready(ctl_))
        {
            btn_start_->show();
            btn_start_->setEnabled(true);
        }
        else if (node::is_active(ctl_))
        {
            btn_stop_->show();
            btn_stop_->setEnabled(true);
        }
    }
}

void auto_ctl_page::update_phase_id(eng::abc::pack args)
{
    if (!args)
    {
        eng::log::info("{}: phase-id = {}", name(), "NULL");
        phase_id_.reset();
    }
    else
    {
        phase_id_ = eng::abc::get<std::uint32_t>(args, 0);
        eng::log::info("{}: phase-id = {}", name(), phase_id_.value());

        execution_error_ = eng::abc::get<bool>(args, 1);
        model_.set_current_phase(*phase_id_);
    }
}

