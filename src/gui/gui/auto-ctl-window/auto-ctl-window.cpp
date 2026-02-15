#include "auto-ctl-window.hpp"

#include "main-page/main-page.hpp"
#include "auto-ctl-page/auto-ctl-page.hpp"
#include "editor-page/editor-page.hpp"

#include "common/program-record.hpp"

auto_ctl_window::auto_ctl_window(QWidget *parent) noexcept
    : QStackedWidget(parent)
{
    main_page_ = new main_page(this);
    connect(main_page_, &main_page::goto_ctl_page, [this](program_record_t const *pr)
    {
        auto_ctl_page_->init(pr);
        QStackedWidget::setCurrentWidget(auto_ctl_page_);
    });
    connect(main_page_, &main_page::goto_editor_page, [this](program_record_t const *pr) {
        editor_page_->init(pr);
        QStackedWidget::setCurrentWidget(editor_page_);
    });
    QStackedWidget::addWidget(main_page_);


    auto_ctl_page_ = new auto_ctl_page(this);
    connect(auto_ctl_page_, &auto_ctl_page::make_done, [this] {
        QStackedWidget::setCurrentWidget(main_page_);
    });
    connect(auto_ctl_page_, &auto_ctl_page::make_edit, [this] {
        QStackedWidget::setCurrentWidget(editor_page_);
    });
    QStackedWidget::addWidget(auto_ctl_page_);


    editor_page_ = new editor_page(this);
    connect(editor_page_, &editor_page::make_done, [this] {
        QStackedWidget::setCurrentWidget(main_page_);
    });
    connect(editor_page_, &editor_page::make_load, [this]
    {
        // auto_ctl_page_->init();
        QStackedWidget::setCurrentWidget(auto_ctl_page_);
    });
    QStackedWidget::addWidget(editor_page_);
}

