#include "auto-mode-window.hpp"
#include "common-page.hpp"
#include "auto-ctl-page.hpp"
#include "editor-page.hpp"

// #include "AutoCtrlWidget.h"
// #include "AutoEditWidget.h"
#include "ProgramModel.h"
// #include "ProgramModelHeader.h"

#include <Widgets/VerticalScroll.h>

#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QScrollBar>

#include <global.h>
#include <aem/log.h>

auto_mode_window::auto_mode_window(QWidget *parent) noexcept
    : QStackedWidget(parent)
    // , axis_cfg_(axis_cfg)
{
    model_ = new ProgramModel();

    common_page_ = new common_page(this);
    connect(common_page_, &common_page::goto_ctl_page, [this](QString const &name)
    {
        auto_ctl_page_->init(name);
        QStackedWidget::setCurrentWidget(auto_ctl_page_);
    });
    connect(common_page_, &common_page::goto_editor_page, [this](QString const &name) {
        editor_page_->init(name);
        QStackedWidget::setCurrentWidget(editor_page_);
    });
    QStackedWidget::addWidget(common_page_);

    auto_ctl_page_ = new auto_ctl_page(this, *model_);
    connect(auto_ctl_page_, &auto_ctl_page::make_done, [this] {
        QStackedWidget::setCurrentWidget(common_page_);
    });
    QStackedWidget::addWidget(auto_ctl_page_);

    editor_page_ = new editor_page(this, *model_);
    connect(editor_page_, &editor_page::make_done, [this] {
        QStackedWidget::setCurrentWidget(common_page_);
    });
    QStackedWidget::addWidget(editor_page_);

    // pTblW_ = new QTableView(this);
    // model_ = new ProgramModel(*pTblW_);
    //
    // QVBoxLayout *vL = new QVBoxLayout(this);
    // vL->setSpacing(0);
    // {
    //     stack_ = new QStackedWidget(this);
    //     stack_->setStyleSheet("border-radius: 10px; background-color: white");
    //     {
    //         autoCtrlWidget_ = new AutoCtrlWidget(this, *model_);
    //         connect(autoCtrlWidget_, &AutoCtrlWidget::make_edit, [this]()
    //         {
    //             connect(pTblW_, SIGNAL(pressed(QModelIndex)), autoEditWidget_, SLOT(tableCellSelect(QModelIndex)));
    //             autoEditWidget_->init();
    //             stack_->setCurrentWidget(autoEditWidget_);
    //         });
    //         connect(autoCtrlWidget_, &AutoCtrlWidget::make_start, [this]() { do_start(); });
    //         connect(autoCtrlWidget_, &AutoCtrlWidget::make_stop, [this] { do_stop(); });
    //         connect(autoCtrlWidget_, &AutoCtrlWidget::load_program, [this] { do_load_program(); });
    //         stack_->addWidget(autoCtrlWidget_);
    //
    //         autoEditWidget_ = new AutoEditWidget(this, *model_);
    //         connect(autoEditWidget_, &AutoEditWidget::make_done, [this]
    //         {
    //             disconnect(pTblW_, SIGNAL(pressed(QModelIndex)), autoEditWidget_, SLOT(tableCellSelect(QModelIndex)));
    //             autoCtrlWidget_->init();
    //             stack_->setCurrentWidget(autoCtrlWidget_);
    //         });
    //         stack_->addWidget(autoEditWidget_);
    //     }
    //     vL->addWidget(stack_);
    // }
    //
    // vL->addSpacing(20);
    //
    // QHBoxLayout *hL = new QHBoxLayout();
    // hL->setSpacing(0);
    // {
    //     VerticalScroll *vs = new VerticalScroll(this);
    //     connect(vs, &VerticalScroll::move, [this](int shift) { make_scroll(shift); });
    //
    //     QVBoxLayout *vL = new QVBoxLayout();
    //     {
    //         pHdrW_ = new QTableWidget(this);
    //         pHdrW_->setStyleSheet("font-size: 9pt; color: #ffffff; background-color: #696969; font-weight: 500");
    //         pHdrW_->setMaximumHeight(79);
    //         pHdrW_->setMinimumHeight(79);
    //         pHdrW_->verticalHeader()->hide();
    //         pHdrW_->horizontalHeader()->hide();
    //         pHdrW_->horizontalHeader()->setStretchLastSection(true);
    //         pHdrW_->setFocusPolicy(Qt::NoFocus);
    //         pHdrW_->setSelectionMode(QAbstractItemView::NoSelection);
    //         pHdrW_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //         pHdrW_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //         vL->addWidget(pHdrW_);
    //
    //         pTblW_->setStyleSheet("font-size: 9pt; color: #696969; font-weight: 500");
    //         pTblW_->verticalHeader()->hide();
    //         pTblW_->horizontalHeader()->hide();
    //         connect(pTblW_->verticalScrollBar(), &QScrollBar::rangeChanged, 
    //                 [vs](int min, int max) { vs->change_range(min, max); });
    //         connect(pTblW_->verticalScrollBar(), &QScrollBar::valueChanged, 
    //                 [vs](int value) { vs->change_value(value); });
    //         pTblW_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //         pTblW_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //         pTblW_->setSelectionMode(QAbstractItemView::NoSelection);
    //         pTblW_->setSortingEnabled(false);
    //         pTblW_->setFocusPolicy(Qt::NoFocus);
    //         pTblW_->horizontalHeader()->setStretchLastSection(true);
    //         pTblW_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    //         pTblW_->setModel(model_);
    //         vL->addWidget(pTblW_);
    //
    //         vL->setStretch(0, 0);
    //         vL->setStretch(1, 1);
    //     }
    //     hL->addLayout(vL);
    //     hL->addWidget(vs);
    // }
    // vL->addLayout(hL);
    //
    // vL->setStretch(0, 0);
    // vL->setStretch(2, 1);
    //
    // sys_key_map_.add("phase-id", autoCtrlWidget_, &AutoCtrlWidget::set_current_phase);
    // sys_key_map_.add("time-front", autoCtrlWidget_, &AutoCtrlWidget::set_time_front);
    // sys_key_map_.add("time-pause", autoCtrlWidget_, &AutoCtrlWidget::set_time_pause);
    // sys_key_map_.add("time-back", autoCtrlWidget_, &AutoCtrlWidget::set_time_back);
    // sys_key_map_.add("running", autoCtrlWidget_, &AutoCtrlWidget::set_running);
    // sys_key_map_.add("pause", autoCtrlWidget_, &AutoCtrlWidget::set_pause);
    //
    // global::subscribe("auto-mode.{}", [this](nlohmann::json::array_t const &keys, nlohmann::json const& value)
    // {
    //     std::string_view kparam = keys[0].get<std::string_view>();
    //
    //     if (kparam == "phase-id")
    //     {
    //         std::size_t phase_id = value.is_null() ? aem::MaxSizeT : value.get<std::size_t>();
    //         autoCtrlWidget_->set_current_phase(phase_id);
    //     }
    //     else
    //     {
    //         sys_key_map_.apply(kparam, value);
    //     }
    // });
    //
    // global::subscribe("cnc.{}.pos", [this](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    // {
    //     try
    //     {
    //         char axisId = keys[0].get<std::string_view>()[0];
    //         autoEditWidget_->set_position(axisId, value.get<float>());
    //     }
    //     catch(...)
    //     {
    //     }
    // });
}

// void AutoCtrlWindow::set_guid(int guid)
// {
//     autoCtrlWidget_->set_guid(guid);
// }

// void AutoCtrlWindow::make_scroll(int shift)
// {
//     auto bar = pTblW_->verticalScrollBar();
//     int pos = bar->sliderPosition();
//     bar->setSliderPosition(pos + shift);
// }

// void AutoCtrlWindow::resizeEvent(QResizeEvent*) noexcept
// {
//     ProgramModelHeader::create_header(model_->prog(), *pHdrW_);
//
//     for (std::size_t i = 0; i < pHdrW_->columnCount(); ++i)
//         pTblW_->setColumnWidth(i, pHdrW_->columnWidth(i));
// }

// void AutoCtrlWindow::apply_axis_cfg() noexcept
// {
// }

// void AutoCtrlWindow::nf_sys_mode(unsigned char sysMode) noexcept
// {
//     autoCtrlWidget_->set_sys_mode(sysMode);
// }

// void AutoCtrlWindow::do_load_program()
// {
//     nlohmann::json program(model_->get_base64_program());
//
//     global::rpc().call("set", { "auto", "program", std::move(program) })
//         .done([this](nlohmann::json const&)
//         {
//             aem::log::info("AutoCtrlWindow::do_load_program: OK");
//         })
//         .error([this](std::string_view emsg)
//         {
//             aem::log::error("AutoCtrlWindow::do_load_program: FAIL {}", emsg);
//         });
//
// }

// void AutoCtrlWindow::do_start()
// {
//     aem::log::info("AutoCtrlWidget::do_start");
//
//     global::rpc().call("start", { "auto" })
//         .done([this](nlohmann::json const&)
//         {
//             aem::log::info("AutoCtrlWindow::on_auto_start: OK");
//         })
//         .error([this](std::string_view emsg)
//         {
//             aem::log::error("AutoCtrlWindow::on_auto_start: {}", emsg);
//         });
// }

// void AutoCtrlWindow::do_stop()
// {
//     global::rpc().call("stop", { "auto" })
//         .done([this](nlohmann::json const&)
//         {
//             aem::log::info("AutoCtrlWindow::on_auto_stop: OK");
//         })
//         .error([this](std::string_view emsg)
//         {
//             aem::log::error("AutoCtrlWindow::on_auto_stop: {}", emsg);
//         });
// }

