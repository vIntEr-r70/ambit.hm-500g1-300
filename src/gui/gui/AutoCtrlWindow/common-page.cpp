#include "common-page.hpp"
#include "common-page-header-widget.hpp"
#include "programs-list-widget.hpp"

#include <QVBoxLayout>
#include <QTabWidget>

common_page::common_page(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        auto w = new common_page_header_widget(this);
        connect(w, &common_page_header_widget::make_load, [this] {
            load_selected_item();
        });
        connect(w, &common_page_header_widget::make_open, [this] {
            open_selected_item();
        });
        w->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        vL->addWidget(w);

        vL->addSpacing(20);

        QTabWidget *tabs_ = new QTabWidget(this);
        {
            QWidget *w = nullptr;

            tabs_->addTab(new programs_list_widget(this), "Список программ");

            w = new QWidget(this);
            tabs_->addTab(w, "Список архивов");
        }
        vL->addWidget(tabs_);
    }
}

void common_page::load_selected_item()
{
    emit goto_ctl_page();
}

void common_page::open_selected_item()
{
    emit goto_editor_page();
}

