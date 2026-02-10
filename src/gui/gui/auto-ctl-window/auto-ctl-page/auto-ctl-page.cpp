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
#include <qnamespace.h>

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
                IconButton *btn_exit = new IconButton(this, ":/editor.to-list");
                connect(btn_exit, &IconButton::clicked, [this] { go_to_main(); });
                btn_exit->setBgColor("#29AC39");
                hL->addWidget(btn_exit);

                IconButton *btn_edit = new IconButton(this, ":/file.edit");
                connect(btn_edit, &IconButton::clicked, [this] { go_to_editor(); });
                btn_edit->setBgColor("#29AC39");
                hL->addWidget(btn_edit);

                hL->addSpacing(30);

                {
                    hL->addWidget(new QLabel("Общее\nвремя", this));
                    QLabel *lbl = new QLabel(this);
                    lbl->setStyleSheet("font-size: 18pt; color: #696969;");
                    lbl->setFrameShape(QFrame::StyledPanel);
                    lbl->setAlignment(Qt::AlignCenter);
                    lbl->setFixedWidth(120);
                    lbl_common_time_ = lbl;
                    hL->addWidget(lbl);
                }

                hL->addSpacing(20);

                {
                    hL->addWidget(new QLabel("Время\nпаузы", this));
                    QLabel *lbl = new QLabel(this);
                    lbl->setStyleSheet("font-size: 18pt; color: #696969;");
                    lbl->setFrameShape(QFrame::StyledPanel);
                    lbl->setAlignment(Qt::AlignCenter);
                    lbl->setFixedWidth(120);
                    lbl_pause_time_ = lbl;
                    hL->addWidget(lbl);
                }

                hL->addSpacing(20);

                lbl_program_name_ = new QLabel(this);
                lbl_program_name_->setStyleSheet("font-size: 14pt; color: #696969;");
                lbl_program_name_->setAlignment(Qt::AlignCenter);
                lbl_program_name_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
                hL->addWidget(lbl_program_name_);

                hL->addSpacing(20);

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

    node::add_input_port_v2("times", [this](eng::abc::pack args) {
        update_times(std::move(args));
    });

    ctl_ = node::add_output_wire();
    node::set_wire_status_handler(ctl_, [this] {
        update_widget_view();
    });

    program_out_ = node::add_output_port("program");

    update_widget_view();
}

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
    // Выводим имя текущей загруженной программы
    lbl_program_name_->setText(model_.name());

    // Задаем программу в модуль режима
    node::set_port_value(program_out_, { model_.get_base64_program() });
}

void auto_ctl_page::make_start()
{
    if (!node::is_ready(ctl_))
        return;

    lbl_common_time_->setText("00:00:00");
    lbl_pause_time_->setText("");

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

constexpr static std::tuple<int, int, int> hms(double sec)
{
    int seconds = static_cast<int>(sec);
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return { h, m, s }; 
}

void auto_ctl_page::update_times(eng::abc::pack args)
{
    double t0 = eng::abc::get<double>(args, 0);

    auto [h1, m1, s1] = hms(t0);
    lbl_common_time_->setText(QString::fromStdString(
        std::format("{:02}:{:02}:{:02}", h1, m1, s1)));

    double t1 = eng::abc::get<double>(args, 1);
    if (std::isnan(t1))
    {
        lbl_pause_time_->setText("");
        return;
    }

    auto [h2, m2, s2] = hms(t1);
    lbl_pause_time_->setText(QString::fromStdString(
        std::format("{:02}:{:02}:{:02}", h2, m2, s2)));
}

