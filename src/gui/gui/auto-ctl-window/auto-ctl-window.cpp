#include "auto-ctl-window.hpp"

#include "InteractWidgets/MessageBox.h"
#include "main-page/main-page.hpp"
#include "auto-ctl-page/auto-ctl-page.hpp"
#include "editor-page/editor-page.hpp"

#include "common/ProgramModel.h"

auto_ctl_window::auto_ctl_window(QWidget *parent) noexcept
    : QStackedWidget(parent)
{
    model_ = new ProgramModel();

    main_page_ = new main_page(this);
    connect(main_page_, &main_page::goto_ctl_page, [this](QString const &name)
    {
        if (!load_program_to_model(name)) return;
        auto_ctl_page_->init();
        QStackedWidget::setCurrentWidget(auto_ctl_page_);
    });
    connect(main_page_, &main_page::goto_editor_page, [this](QString const &name) {
        editor_page_->init(name);
        QStackedWidget::setCurrentWidget(editor_page_);
    });
    QStackedWidget::addWidget(main_page_);


    auto_ctl_page_ = new auto_ctl_page(this, *model_);
    connect(auto_ctl_page_, &auto_ctl_page::make_done, [this] {
        QStackedWidget::setCurrentWidget(main_page_);
    });
    connect(auto_ctl_page_, &auto_ctl_page::make_edit, [this] {
        QStackedWidget::setCurrentWidget(editor_page_);
    });
    QStackedWidget::addWidget(auto_ctl_page_);


    editor_page_ = new editor_page(this, *model_);
    connect(editor_page_, &editor_page::make_done, [this] {
        QStackedWidget::setCurrentWidget(main_page_);
    });
    QStackedWidget::addWidget(editor_page_);

    msg_ = new MessageBox(this, MessageBox::HeadError);
    msg_->set_message("Не удалось загрузить программу");
}

bool auto_ctl_window::load_program_to_model(QString const &name)
{
    if (!model_->load_from_local_file(name))
    {
        msg_->show();
        return false;
    }

    return true;
}
