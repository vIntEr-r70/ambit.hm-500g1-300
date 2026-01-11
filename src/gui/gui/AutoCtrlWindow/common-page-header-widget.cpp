#include "common-page-header-widget.hpp"

#include <Widgets/ValueViewString.h>
// #include <Widgets/ValueViewTime.h>
#include <Widgets/RoundButton.h>
// #include <Widgets/IconButton.h>

// #include "ProgramListDlg.h"
// #include "ProgramModel.h"

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>

common_page_header_widget::common_page_header_widget(QWidget *parent) noexcept
    : QWidget(parent)
{
    setObjectName("common_page_header_widget");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#common_page_header_widget { border-radius: 20px; background-color: white; }");

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 0);
    effect->setColor(Qt::gray);
    effect->setBlurRadius(20);
    setGraphicsEffect(effect);

    QVBoxLayout *vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            name_ = new ValueViewString(this);
            name_->setTitle("Название программы");
            name_->setMaximumWidth(400);
            name_->setMinimumWidth(400);
            hL->addWidget(name_);

            hL->addStretch();

            RoundButton *btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_open(); });
            btn->setText("Создать");
            btn->setBgColor("#29AC39");
            // btn->setMinimumWidth(150);
            // btn_start_ = btn;
            hL->addWidget(btn);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_open(); });
            btn->setText("Изменить");
            btn->setBgColor("#29AC39");
            // btn->setMinimumWidth(150);
            // btn_start_ = btn;
            hL->addWidget(btn);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { emit make_load(); });
            btn->setText("Загрузить");
            btn->setBgColor("#29AC39");
            // btn->setMinimumWidth(150);
            // btn_start_ = btn;
            hL->addWidget(btn);

            btn = new RoundButton(this);
            // connect(btn, &RoundButton::clicked, [this] { emit make_stop(); });
            btn->setText("Удалить");
            btn->setBgColor("#E55056");
            // btn->setMinimumWidth(100);
            // btn_stop_ = btn;
            hL->addWidget(btn);
            // hL->addSpacing(20);

            // btn_load_ = new IconButton(this, ":/LoadFile");
            // // connect(btn_load_, &IconButton::clicked, [this] { list_dlg_->show(); });
            // btn_load_->setBgColor("#8a8a8a");
            // hL->addWidget(btn_load_);

            // btn_edit_ = new IconButton(this, ":/EditFile");
            // connect(btn_edit_, &IconButton::clicked, [this] 
            // { 
            //     emit make_edit(); 
            // });
            // btn_edit_->setBgColor("#8a8a8a");
            // btn_edit_->setVisible(false);
            // hL->addWidget(btn_edit_);

            // btn_new_ = new IconButton(this, ":/editor.new");
            // connect(btn_new_, &IconButton::clicked, [this] 
            // { 
            //     model_.reset();
            //     emit make_edit(); 
            // });
            // btn_new_->setBgColor("#8a8a8a");
            // hL->addWidget(btn_new_);

            // hL->addStretch();
        }
        vL->addLayout(hL);

        comments_ = new ValueViewString(this);
        comments_->setTitle("Коментарии");
        vL->addWidget(comments_);
    }

        // vL = new QVBoxLayout();
        // {
        //     QHBoxLayout *hL = new QHBoxLayout();
        //     {
        //         RoundButton *btn = new RoundButton(this);
        //         // connect(btn, &RoundButton::clicked, [this] { emit make_start(); });
        //         btn->setText("Загрузить");
        //         btn->setBgColor("#29AC39");
        //         // btn->setMinimumWidth(150);
        //         // btn_start_ = btn;
        //         hL->addWidget(btn);
        //
        //         btn = new RoundButton(this);
        //         // connect(btn, &RoundButton::clicked, [this] { emit make_stop(); });
        //         btn->setText("Удалить");
        //         btn->setBgColor("#E55056");
        //         // btn->setMinimumWidth(100);
        //         // btn_stop_ = btn;
        //         hL->addWidget(btn);
        //     }
        //     vL->addLayout(hL);
        //
        //     vL->addStretch();
        //
        //     hL = new QHBoxLayout();
        //     {
        //         // time_front_ = new ValueViewTime(this, "Работа");
        //         // hL->addWidget(time_front_);
        //         // time_pause_ = new ValueViewTime(this, "Пауза");
        //         // hL->addWidget(time_pause_);
        //         /* time_back_ = new ValueViewTime(this, "Осталось"); */
        //         /* hL->addWidget(time_back_); */
        //     }
        //     vL->addLayout(hL);
        // }
        // hL->addLayout(vL);
    // hL->setStretch(0, 1);

    // list_dlg_ = new ProgramListDlg(this);
    // connect(list_dlg_, SIGNAL(makeLoadFromLocalFile(QString)), this, SLOT(on_load_local_program(QString)));
}

// void AutoCtrlWidget::init()
// {
//     name_->set_value("");
//     comments_->set_value("");
//
//     btn_edit_->setVisible(false);
//
//     if (model_.name().isEmpty())
//         return;
//
//     if (!model_.load_from_local_file(model_.name()))
//         return;
//
//     btn_edit_->setVisible(true);
//
//     name_->set_value(model_.name());
//     comments_->set_value(model_.comments());
//
//     emit load_program();
// }

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

