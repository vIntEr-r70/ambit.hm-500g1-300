#include "editor-page-header-widget.hpp"
#include "ProgramModel.h"
#include "EditorMessageBox.h"
#include "ProgramModelHeader.h"

#include <Widgets/ValueSetString.h>
#include <Widgets/IconButton.h>

#include <QVBoxLayout>
#include <QTableView>

#include <aem/log.h>

editor_page_header_widget::editor_page_header_widget(QWidget *parent, ProgramModel &model) noexcept
    : QWidget(parent)
    , model_(model)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            name_ = new ValueSetString(this);
            connect(name_, &ValueSetString::onValueChanged, [this] 
            { 
                model_.set_name(name_->value());
                need_save(true); 
            });
            name_->setTitle("Название программы");
            name_->setMaximumWidth(400);
            name_->setMinimumWidth(400);
            hL->addWidget(name_);

            hL->addSpacing(50);

            IconButton *btn_exit = new IconButton(this, ":/check-mark");
            connect(btn_exit, &IconButton::clicked, [this] { emit make_done();/*do_exit();*/ });
            btn_exit->setBgColor("#8a8a8a");
            hL->addWidget(btn_exit);

            btn_save_ = new IconButton(this, ":/SaveFile");
            connect(btn_save_, &IconButton::clicked, [this] { do_save();});
            btn_save_->setBgColor("#8a8a8a");
            hL->addWidget(btn_save_);

            hL->addStretch();

            QList<QPair<QString, TableAc>> tableAcBtns = {
                { ":/editor.main-op",       AddMainOp     },
                { ":/editor.main-rel-op",   AddRelMainOp  },
                { ":/editor.pause",         AddPauseOp    },
                { ":/editor.inductor",      AddTimedFCOp  },
                { ":/editor.center",        AddCenterOp   },
                { ":/AddGoTo",              AddGoToOp     },
                { ":/PhaseRemove",          DeleteOp      }
            };

            for (auto const& ac : tableAcBtns)
            {
                IconButton *btn = new IconButton(this, ac.first, ac.second);
                btn->setBgColor("#8a8a8a");
                connect(btn, &IconButton::clickedId, [this](int op) { make_table_op(static_cast<TableAc>(op)); });
                hL->addWidget(btn);
            }
        }
        vL->addLayout(hL);

        comments_ = new ValueSetString(this);
        connect(comments_, &ValueSetString::onValueChanged, [this] 
        { 
            model_.set_comments(comments_->value());
            need_save(true); 
        });
        comments_->setTitle("Коментарии");
        vL->addWidget(comments_);
    }

    msg_ = new EditorMessageBox(this);
}

void editor_page_header_widget::init()
{
    name_->set_value(model_.name());
    comments_->set_value(model_.comments());

    need_save(false);
}

void editor_page_header_widget::make_table_op(TableAc ac) noexcept
{
    need_save(true);

    switch(ac)
    {
    case AddMainOp:
        model_.add_main_op(true);
        break;
    case AddRelMainOp:
        model_.add_main_op(false);
        break;
    case DeleteOp:
        model_.remove_op();
        break;
    case AddPauseOp:
        model_.add_op(program::op_type::pause);
        break;
    case AddGoToOp:
        model_.add_op(program::op_type::loop);
        break;
    case AddTimedFCOp:
        model_.add_op(program::op_type::fc);
        break;
    case AddCenterOp:
        model_.add_op(program::op_type::center);
        break;
    }

    emit rows_count_changed(false);
}

void editor_page_header_widget::need_save(bool dirty) noexcept
{
    btn_save_->setVisible(dirty);
}

void editor_page_header_widget::do_exit() noexcept
{
    if (btn_save_->isVisible())
    {
        msg_->show_question("Продолжить без сохранения изменений?", [this]
        {
            emit make_done();
        });
    }
    else
    {
        emit make_done();
    }
}

void editor_page_header_widget::do_save() noexcept
{
    // TODO: touch
    if (model_.name().isEmpty())
    {
        name_->external_touch();
        return;
    }

    if (!model_.save_to_file())
    {
        msg_->show_error("Не удалось сохранить программу!");
        return;
    }

    need_save(false);
}

