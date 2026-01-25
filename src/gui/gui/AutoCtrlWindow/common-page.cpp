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

        connect(w, &common_page_header_widget::make_create_program,
                [this] { make_create_program(); });
        connect(w, &common_page_header_widget::make_edit_program,
                [this] { make_edit_program(); });
        connect(w, &common_page_header_widget::make_init_program,
                [this] { make_init_program(); });

        w->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        vL->addWidget(w);

        vL->addSpacing(20);

        QTabWidget *tabs_ = new QTabWidget(this);
        {
            QWidget *w = nullptr;

            programs_list_widget_ = new programs_list_widget(this);
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

