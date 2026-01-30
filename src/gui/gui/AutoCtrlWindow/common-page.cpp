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
        header_ = new common_page_header_widget(this);

        connect(header_, &common_page_header_widget::make_create_program,
                [this] { make_create_program(); });
        connect(header_, &common_page_header_widget::make_edit_program,
                [this] { make_edit_program(); });
        connect(header_, &common_page_header_widget::make_init_program,
                [this] { make_init_program(); });
        connect(header_, &common_page_header_widget::make_remove_program,
                [this] { make_remove_program(); });

        header_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        vL->addWidget(header_);

        vL->addSpacing(20);

        QTabWidget *tabs_ = new QTabWidget(this);
        {
            QWidget *w = nullptr;

            programs_list_widget_ = new programs_list_widget(this);
            connect(programs_list_widget_, &programs_list_widget::row_changed,
                    [this] { selected_program_changed(); });
            tabs_->addTab(programs_list_widget_, "Список программ");

            w = new QWidget(this);
            tabs_->addTab(w, "Список архивов");
        }
        vL->addWidget(tabs_);
    }
}

void common_page::make_create_program()
{
    emit goto_editor_page("");
}

void common_page::make_edit_program()
{
    QString name = programs_list_widget_->current();
    if (!name.isEmpty())
        emit goto_editor_page(name);
}

// Открываем программу для исполнения
void common_page::make_init_program()
{
    QString name = programs_list_widget_->current();
    if (!name.isEmpty())
        emit goto_ctl_page(name);
}

void common_page::make_remove_program()
{
    programs_list_widget_->remove_selected();
}

void common_page::selected_program_changed()
{
    QString name = programs_list_widget_->current();
    header_->set_program_name(name);
    // emit program_changed(name);
}

