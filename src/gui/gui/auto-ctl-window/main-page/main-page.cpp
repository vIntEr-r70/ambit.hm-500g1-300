#include "main-page.hpp"
#include "main-page-header-widget.hpp"
#include "programs-list-widget.hpp"

#include <QVBoxLayout>
#include <QTabWidget>

main_page::main_page(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        header_ = new main_page_header_widget(this);

        connect(header_, &main_page_header_widget::make_create_program,
                [this] { make_create_program(); });
        connect(header_, &main_page_header_widget::make_edit_program,
                [this] { make_edit_program(); });
        connect(header_, &main_page_header_widget::make_init_program,
                [this] { make_init_program(); });
        connect(header_, &main_page_header_widget::make_remove_program,
                [this] { programs_list_widget_->remove_selected(); });

        header_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        vL->addWidget(header_);

        vL->addSpacing(20);

        QTabWidget *tabs_ = new QTabWidget(this);
        {
            QWidget *w = nullptr;

            programs_list_widget_ = new programs_list_widget(this, header_);
            tabs_->addTab(programs_list_widget_, "Список программ");

            w = new QWidget(this);
            tabs_->addTab(w, "Список архивов");
        }
        vL->addWidget(tabs_);
    }
}

void main_page::make_create_program()
{
    emit goto_editor_page(nullptr);
}

void main_page::make_edit_program()
{
    emit goto_editor_page(programs_list_widget_->selected_record());
}

void main_page::make_init_program()
{
    emit goto_ctl_page(programs_list_widget_->selected_record());
}

