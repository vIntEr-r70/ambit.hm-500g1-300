#include "manual-engine-set-dlg.h"

#include <Widgets/RoundButton.h>
#include <Widgets/NumberCalcField.h>
#include <InteractWidgets/KeyboardWidget.h>

// #include <axis-cfg.h>
#include <UnitsCalc.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QDoubleValidator>
#include <QStackedWidget>


#include <aem/log.h>

manual_engine_set_dlg::manual_engine_set_dlg(QWidget* parent)
    : InteractWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("border-radius: 20px; background-color: #c8c8c8");

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    {
        stack_ = new QStackedWidget(this);
        stack_->addWidget(create_move_to_page());
        stack_->addWidget(create_calibration_page());

        vL->addWidget(stack_);
    }
}

void manual_engine_set_dlg::set_axis_name(QString const &value)
{
    title_[0]->setText(value);
}

void manual_engine_set_dlg::showEvent(QShowEvent*)
{
    le_->setFocus();
    le_->selectAll();
}

void manual_engine_set_dlg::onKeyPressed(int key)
{
    KeyboardWidget::sendKeyPressed(le_, key, buttons_[key]->text());
}

void manual_engine_set_dlg::onACKey()
{
    move_to_pos_->acc();
}

void manual_engine_set_dlg::onBackspaceKey()
{
    KeyboardWidget::sendKeyPressed(le_, Qt::Key_Backspace);
}

// void manual_engine_set_dlg::set_calibrate_axis(char calibrateId) noexcept
// {
//     calibrateId_ = calibrateId;
//     update_calibrate_status();
//
//     if (calibrateId == 0)
//     {
//         if (calibrate_timer_id_ != -1)
//         {
//             killTimer(calibrate_timer_id_);
//             calibrate_timer_id_ = -1;
//         }
//         return;
//     }
//
//     if (calibrate_timer_id_ == -1)
//         calibrate_timer_id_ = startTimer(500);
//
//     on_calibrate_timer();
// }

// void manual_engine_set_dlg::set_calibrate_step(int step) noexcept
// {
//     aem::log::warn("ManualEngineSetDlg::set_calibrate_step: {}", step);
//
//     // Калибровка успешно завершена, закрываем окно
//     if (step == 0)
//     {
//         InteractWidget::hide();
//         return;
//     }
//
//     std::size_t ustep = std::labs(step);
//
//     lbl_calibrate_steps_[calibrate_step_]->setStyleSheet("color: white; font: 14pt;");
//
//     calibrate_step_ = ustep - 1;
//     calibrate_blink_ = false;
//     calibrate_error_ = step < 0;
//
//     if (calibrateId_ == 0)
//     {
//         if (calibrate_timer_id_ != -1)
//         {
//             killTimer(calibrate_timer_id_);
//             calibrate_timer_id_ = -1;
//         }
//         return;
//     }
//
//     if (calibrate_timer_id_ == -1)
//         calibrate_timer_id_ = startTimer(500);
//
//     on_calibrate_timer();
// }

// void manual_engine_set_dlg::on_calibrate_timer() noexcept
// {
//     if (calibrate_error_)
//     {
//         lbl_calibrate_steps_[calibrate_step_]->setStyleSheet("color: red; font: 14pt;");
//         return;
//     }
//
//     calibrate_blink_ = !calibrate_blink_;
//
//     lbl_calibrate_steps_[calibrate_step_]->
//         setStyleSheet(QString("color: %1; font: 14pt;").arg(calibrate_blink_ ? "blue" : "white"));
// }

void manual_engine_set_dlg::prepare(char axisId) noexcept
{
    // axisId_ = axisId;
    // auto &&desc = axis_cfg_.axis(axisId);

    // update_calibrate_status();


    // move_to_pos_->init(QString::number(pos(), 'f', 1));

    // stack_->setCurrentIndex(0);
}

// void manual_engine_set_dlg::to_calibration_window() noexcept
// {
//     // auto &&desc = axis_cfg_.axis(axisId_);
//
//     // title_[1]->setText(desc.name);
//
//     update_calibrate_status();
//
//     // lbl_calibrate_info_[1]->setText(QString("Домашний концевик: %1").arg(desc.home ? "MAX" : "MIN"));
//     // lbl_calibrate_info_[2]->setText(QString("Длинна: %1 мм").arg(desc.length, 0, 'f', 0));
//     lbl_calibrate_info_[3]->setText("Ошибка позиционирования: --- мм");
//
//     stack_->setCurrentIndex(1);
// }

void manual_engine_set_dlg::set_position(char axisId, float pos) noexcept
{
    axis_pos_[axisId] = pos;

    // Если не наша ось, игнорируем
    if (axisId_ != axisId)
        return;

    lbl_calibrate_info_[0]->setText(QString("Текущая позиция: %1 мм").arg(pos, 0, 'f', 1));

    if (stack_->currentIndex() == 0)
        move_to_pos_->init(QString::number(pos, 'f', 1));
}

// void manual_engine_set_dlg::update_calibrate_status() noexcept
// {
//     if (calibrateId_ == 0)
//     {
//         start_calibration_->setEnabled(true);
//         lbl_calibrate_status_->setText("Будет выполнена калибровка данной оси");
//         lbl_calibrate_status_->setStyleSheet("color: green; font: bold 12pt");
//     }
//     else if (calibrateId_ == axisId_)
//     {
//         start_calibration_->setEnabled(false);
//         lbl_calibrate_status_->setText("Выполняется калибровка данной оси");
//         lbl_calibrate_status_->setStyleSheet("color: blue; font: bold 12pt");
//     }
//     else
//     {
//         start_calibration_->setEnabled(false);
//         lbl_calibrate_status_->setText("Выполняется калибровка другой оси");
//         lbl_calibrate_status_->setStyleSheet("color: yellow; font: bold 12pt");
//     }
// }

void manual_engine_set_dlg::onEnterBtn()
{
    emit apply_pos(move_to_pos_->result());

    // auto &&desc = axis_cfg_.axis(axisId_);
    // num = UnitsCalc::fromPos(desc.muGrads, num);
    // InteractWidget::hide();
}

void manual_engine_set_dlg::onCancelBtn()
{
    emit apply_axis('\0');
    InteractWidget::hide();
}

float manual_engine_set_dlg::pos() const noexcept
{
    auto it = axis_pos_.find(axisId_);
    return it == axis_pos_.end() ? 0.0f : it->second; 
}

QWidget* manual_engine_set_dlg::create_move_to_page() noexcept
{
    QWidget *w = new QWidget(this);
    QVBoxLayout *vL = new QVBoxLayout(w);
    vL->setContentsMargins(0, 0, 0, 0);
    vL->setSpacing(30);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            title_[0] = new QLabel(w);
            title_[0]->setStyleSheet("color: white; font: bold 20pt");
            hL->addWidget(title_[0]);

            hL->addStretch();

            btn_to_zero_ = new RoundButton(w);
            // connect(btn_to_zero_, &RoundButton::clicked, [this] { emit doZero(axisId_); });
            btn_to_zero_->setText("Сбросить в 0");
            btn_to_zero_->setBgColor(QColor("#29AC39"));
            btn_to_zero_->setTextColor(Qt::white);
            hL->addWidget(btn_to_zero_);

            btn_to_calibrate_ = new RoundButton(w);
            // connect(btn_to_calibrate_, &RoundButton::clicked, [this] { to_calibration_window(); });
            btn_to_calibrate_->setText("Выполнить калибровку");
            btn_to_calibrate_->setBgColor(QColor("#29AC39"));
            btn_to_calibrate_->setTextColor(Qt::white);
            hL->addWidget(btn_to_calibrate_);
        }
        vL->addLayout(hL);

        hL = new QHBoxLayout();
        {
            QVBoxLayout* vL = new QVBoxLayout();
            vL->setSpacing(5);
            {
                vL->addStretch();

                move_to_pos_ = new NumberCalcField(w);
                connect(move_to_pos_, &NumberCalcField::result_changed, 
                    [this] (bool allow) { apply_new_pos_->setEnabled(allow); });
                vL->addWidget(move_to_pos_);
                le_ = move_to_pos_->le_;

                vL->addStretch();

                apply_new_pos_ = new RoundButton(w);
                connect(apply_new_pos_, SIGNAL(clicked()), this, SLOT(onEnterBtn()));
                apply_new_pos_->setBgColor(QColor(0x29AC39));
                apply_new_pos_->setIcon(":/check-mark");
                vL->addWidget(apply_new_pos_);
            }
            hL->addLayout(vL);

            vL = new QVBoxLayout();
            vL->setSpacing(20);
            {
                std::array<QPair<QString, int>, 12> const syms = {{
                    { "7", Qt::Key_7 },     { "8", Qt::Key_8 }, { "9", Qt::Key_9 },
                    { "4", Qt::Key_4 },     { "5", Qt::Key_5 }, { "6", Qt::Key_6 },
                    { "1", Qt::Key_1 },     { "2", Qt::Key_2 }, { "3", Qt::Key_3 },
                    { ".", Qt::Key_Period}, { "0", Qt::Key_0 }, { "-", Qt::Key_Minus }
                }};

                QGridLayout* gL = new QGridLayout();
                gL->setSpacing(20);
                {
                    for (std::size_t i = 0; i < syms.size(); ++i)
                    {
                        auto const& sym = syms[i];
                        if (sym.second == 0)
                            continue;

                        RoundButton *btn = KeyboardWidget::createKeyBtn(w, sym.second);
                        connect(btn, SIGNAL(clickedId(int)), this, SLOT(onKeyPressed(int)));
                        btn->setText(sym.first);
                        buttons_[sym.second] = btn;
                        gL->addWidget(btn, i / 3, i % 3);
                    }
                }
                vL->addLayout(gL);
            }
            hL->addLayout(vL);

            vL = new QVBoxLayout();
            vL->setSpacing(20);
            {
                RoundButton* btn = KeyboardWidget::createKeyBtn(w);
                btn->setText("AC");
                connect(btn, SIGNAL(clicked()), this, SLOT(onACKey()));
                btn->setMinimumWidth(90);
                vL->addWidget(btn);

                btn = KeyboardWidget::createKeyBtn(w);
                connect(btn, SIGNAL(clicked()), this, SLOT(onBackspaceKey()));
                btn->setIcon(":/kbd.dlg.undo");
                btn->setMinimumWidth(90);
                vL->addWidget(btn);

                vL->addStretch();

                btn = new RoundButton(w);
                connect(btn, SIGNAL(clicked()), this, SLOT(onCancelBtn()));
                btn->setIcon(":/kbd.dlg.cancel");
                btn->setBgColor("#E55056");
                btn->setMinimumWidth(90);
                vL->addWidget(btn);
            }
            hL->addLayout(vL);
        }
        vL->addLayout(hL);
    }

    return w;
}

QWidget* manual_engine_set_dlg::create_calibration_page() noexcept
{
    QWidget *w = new QWidget(this);

    QHBoxLayout* hL = new QHBoxLayout(w);
    hL->setContentsMargins(0, 0, 0, 0);
    hL->setSpacing(30);

    QVBoxLayout* vL = new QVBoxLayout();
    vL->setSpacing(5);
    {
        title_[1] = new QLabel(w);
        title_[1]->setStyleSheet("color: white; font: bold 20pt");
        vL->addWidget(title_[1]);

        lbl_calibrate_status_ = new QLabel(w);
        vL->addWidget(lbl_calibrate_status_);

        vL->addStretch();

        for (auto &lbl : lbl_calibrate_info_)
        {
            lbl = new QLabel(w);
            lbl->setStyleSheet("color: white;");
            vL->addWidget(lbl);
        }

        vL->addStretch();

        start_calibration_ = new RoundButton(w);
        connect(start_calibration_, &RoundButton::clicked, [this]
        {
            start_calibration_->setEnabled(false);
            emit doCalibrate(axisId_);
        });
        start_calibration_->setBgColor(QColor(0x29AC39));
        start_calibration_->setIcon(":/check-mark");
        vL->addWidget(start_calibration_);
    }
    hL->addLayout(vL);

    vL = new QVBoxLayout();
    vL->setSpacing(15);
    {
        QLabel *lbl = new QLabel(w);
        lbl->setStyleSheet("color: white; font: bold 16pt");
        lbl->setText("Статус процесса");
        lbl->setAlignment(Qt::AlignCenter);
        vL->addWidget(lbl);

        vL->addStretch();

        for (auto &lbl : lbl_calibrate_steps_)
        {
            lbl = new QLabel(w);
            lbl->setWordWrap(true);
            lbl->setStyleSheet("color: white; font: 14pt;");
            vL->addWidget(lbl);
        }
        lbl_calibrate_steps_[0]->setText("Поиск домашнего концевика");
        lbl_calibrate_steps_[1]->setText("Откат от концевика");
        lbl_calibrate_steps_[2]->setText("Повторный поиск c меньшей скоростью");
        lbl_calibrate_steps_[3]->setText("Обновление значения позиции драйвера");

        vL->addStretch();

        RoundButton *btn = new RoundButton(w);
        connect(btn, &RoundButton::clicked, [this] 
        { 
            emit doCalibrate('\0'); 
            InteractWidget::hide();
        });
        btn->setIcon(":/kbd.dlg.cancel");
        btn->setBgColor("#E55056");
        btn->setMinimumWidth(90);
        vL->addWidget(btn);
    }
    hL->addLayout(vL);

    hL->setStretch(0, 1);

    return w;
}

