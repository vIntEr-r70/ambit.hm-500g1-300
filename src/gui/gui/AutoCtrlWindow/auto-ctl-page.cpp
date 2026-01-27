#include "auto-ctl-page.hpp"

#include <Widgets/ValueViewString.h>
#include <Widgets/ValueViewTime.h>
#include <Widgets/RoundButton.h>
#include <Widgets/IconButton.h>
#include <Widgets/VerticalScroll.h>

#include "ProgramModel.h"
#include "eng/sibus/sibus.hpp"
#include "program-widget.hpp"

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QTableWidget>

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
            w->setObjectName("auto_ctl_header");
            w->setAttribute(Qt::WA_StyledBackground, true);
            w->setStyleSheet("QWidget#auto_ctl_header { border-radius: 20px; background-color: white; }");

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

        program_widget_ = new program_widget(this, model);
        vL->addWidget(program_widget_);
    }

    node::add_input_port_v2("phase-id", [this](eng::abc::pack args)
    {
        update_phase_id(std::move(args));
    });

    ctl_ = node::add_output_wire();
    node::set_wire_response_handler(ctl_, [this](bool success, eng::abc::pack args)
    {
        if (success)
            eng::log::info("CMD DONE SUCCESS");
        else
            eng::log::error("CMD DONE FAILED: {}", eng::abc::get_sv(args));
    });

    node::set_wire_status_handler(ctl_, [this](eng::sibus::wire_status)
    {
        update_widget_view();
    });

    update_widget_view();



    // QHBoxLayout *hL = new QHBoxLayout(this);
    // {
    //     QVBoxLayout *vL = new QVBoxLayout();
    //     {
    //         QHBoxLayout *hL = new QHBoxLayout();
    //         {
    //             name_ = new ValueViewString(this);
    //             name_->setTitle("Название программы");
    //             name_->setMaximumWidth(400);
    //             name_->setMinimumWidth(400);
    //             hL->addWidget(name_);
    //
    //             hL->addSpacing(20);
    //
    //             btn_load_ = new IconButton(this, ":/LoadFile");
    //             // connect(btn_load_, &IconButton::clicked, [this] { list_dlg_->show(); });
    //             btn_load_->setBgColor("#8a8a8a");
    //             hL->addWidget(btn_load_);
    //
    //             btn_edit_ = new IconButton(this, ":/EditFile");
    //             connect(btn_edit_, &IconButton::clicked, [this] 
    //             { 
    //                 emit make_edit(); 
    //             });
    //             btn_edit_->setBgColor("#8a8a8a");
    //             btn_edit_->setVisible(false);
    //             hL->addWidget(btn_edit_);
    //
    //             btn_new_ = new IconButton(this, ":/editor.new");
    //             connect(btn_new_, &IconButton::clicked, [this] 
    //             { 
    //                 model_.reset();
    //                 emit make_edit(); 
    //             });
    //             btn_new_->setBgColor("#8a8a8a");
    //             hL->addWidget(btn_new_);
    //
    //             hL->addStretch();
    //         }
    //         vL->addLayout(hL);
    //
    //         comments_ = new ValueViewString(this);
    //         comments_->setTitle("Коментарии");
    //         vL->addWidget(comments_);
    //     }
    //     hL->addLayout(vL);
    //
    //     vL = new QVBoxLayout();
    //     {
    //         QHBoxLayout *hL = new QHBoxLayout();
    //         {
    //             RoundButton *btn = new RoundButton(this);
    //             connect(btn, &RoundButton::clicked, [this] { emit make_start(); });
    //             btn->setText("СТАРТ/ДАЛЕЕ");
    //             btn->setBgColor("#29AC39");
    //             btn->setMinimumWidth(150);
    //             btn_start_ = btn;
    //             hL->addWidget(btn);
    //
    //             btn = new RoundButton(this);
    //             connect(btn, &RoundButton::clicked, [this] { emit make_stop(); });
    //             btn->setText("СТОП");
    //             btn->setBgColor("#E55056");
    //             btn->setMinimumWidth(100);
    //             btn_stop_ = btn;
    //             hL->addWidget(btn);
    //         }
    //         vL->addLayout(hL);
    //
    //         vL->addStretch();
    //
    //         hL = new QHBoxLayout();
    //         {
    //             time_front_ = new ValueViewTime(this, "Работа");
    //             hL->addWidget(time_front_);
    //             time_pause_ = new ValueViewTime(this, "Пауза");
    //             hL->addWidget(time_pause_);
    //             /* time_back_ = new ValueViewTime(this, "Осталось"); */
    //             /* hL->addWidget(time_back_); */
    //         }
    //         vL->addLayout(hL);
    //     }
    //     hL->addLayout(vL);
    // }
    // hL->setStretch(0, 1);

    // list_dlg_ = new ProgramListDlg(this);
    // connect(list_dlg_, SIGNAL(makeLoadFromLocalFile(QString)), this, SLOT(on_load_local_program(QString)));
}

void auto_ctl_page::init(QString const &name)
{
    model_.load_from_local_file(name);
    program_widget_->rows_count_changed(true);

    // Передаем программу в узел автоматического режима
    std::string base64 = model_.get_base64_program();
    eng::log::info("{}", base64);
    node::send_wire_signal(ctl_, { "upload-program", base64 });

    if (phase_id_.has_value())
        model_.set_current_row(*phase_id_);

    // Ждем пока произойдет прием и проверка программы на корректность
    // после чего загружаем ее себе и позволяем пользователю запустить
    // режим на выполнение




    // name_->set_value("");
    // comments_->set_value("");

    // btn_edit_->setVisible(false);

    // if (model_.name().isEmpty())
    //     return;

    // if (!model_.load_from_local_file(model_.name()))
    //     return;
    //
    // btn_edit_->setVisible(true);
    //
    // name_->set_value(model_.name());
    // comments_->set_value(model_.comments());
    //
    // emit load_program();
}

void auto_ctl_page::make_start()
{
    btn_start_->setEnabled(false);
    node::activate(ctl_, { });
}

void auto_ctl_page::make_stop()
{
    node::deactivate(ctl_);
}

void auto_ctl_page::update_widget_view()
{
    eng::log::info("auto_ctl_page::update_widget_view: ws = {}",
            static_cast<std::uint8_t>(node::wire(ctl_)));

    btn_stop_->hide();
    btn_start_->hide();
    btn_continue_->hide();

    if (node::is_wire_usable(ctl_))
    {
        btn_start_->show();
        btn_start_->setEnabled(true);
    }
    else if (node::wire(ctl_) == eng::sibus::wire_status::active)
    {
        btn_stop_->show();
    }
    else
    {
        btn_start_->show();
        btn_start_->setEnabled(false);
    }

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
    if (!args)
    {
        phase_id_.reset();
    }
    else
    {
        phase_id_ = eng::abc::get<std::uint32_t>(args, 0);
        execution_error_ = eng::abc::get<bool>(args, 1);
        // ctl_mode_ = eng::abc::get<std::uint32_t>(args, 1);
        model_.set_current_row(*phase_id_);
    }

    update_widget_view();
}

// void AutoCtrlWidget::set_guid(int guid)
// {
//     // list_dlg_->set_guid(guid);
// }

// void AutoCtrlWidget::set_sys_mode(unsigned char sysMode) noexcept
// {
//     mode_allow_ = (sysMode == Core::SysMode::Auto);
//     update_ctl_buttons();
//
//     time_front_->setEnabled(mode_allow_);
//     /* time_back_->setEnabled(allow); */
//     time_pause_->setEnabled(mode_allow_);
// }

// void AutoCtrlWidget::set_current_phase(std::size_t phase_id) noexcept
// {
//     model_.set_current_phase(phase_id);
//
//     btn_load_->setVisible(phase_id == aem::MaxSizeT);
//     btn_edit_->setVisible(phase_id == aem::MaxSizeT && !model_.name().isEmpty());
//     btn_new_->setVisible(phase_id == aem::MaxSizeT);
// }

// void AutoCtrlWidget::set_time_front(std::size_t sec) noexcept
// {
//     time_front_->set_value(sec);
// }

// void AutoCtrlWidget::set_time_back(std::size_t sec) noexcept
// {
//     /* time_back_->set_value(sec); */
// }

// void AutoCtrlWidget::set_time_pause(std::size_t sec) noexcept
// {
//     time_pause_->set_value(sec);
// }

// void AutoCtrlWidget::set_running(bool running) noexcept
// {
//     running_ = running;
//     update_ctl_buttons();
// }

// void AutoCtrlWidget::set_pause(bool pause) noexcept
// {
//     in_pause_ = pause;
//     update_ctl_buttons();
// }

// void AutoCtrlWidget::on_load_local_program(QString name)
// {
//     model_.set_name(name);
//     init();
// }

// void AutoCtrlWidget::update_ctl_buttons()
// {
//     btn_start_->setEnabled(mode_allow_ && (!running_ || in_pause_));
//     btn_stop_->setEnabled(mode_allow_ && running_);
// }
