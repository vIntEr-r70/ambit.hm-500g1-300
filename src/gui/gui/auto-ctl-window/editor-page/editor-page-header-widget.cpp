#include "editor-page-header-widget.hpp"
#include "../common/ProgramModel.h"
#include "../common/ProgramModelHeader.h"
#include "EditorMessageBox.h"

#include <Widgets/ValueSetString.h>
#include <Widgets/IconButton.h>

#include <QVBoxLayout>
#include <QTableView>
#include <QGraphicsDropShadowEffect>

#include <eng/log.hpp>

editor_page_header_widget::editor_page_header_widget(QWidget *parent, ProgramModel &model) noexcept
    : QWidget(parent)
    , model_(model)
{
    setObjectName("editor_page_header_widget");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#editor_page_header_widget { border-radius: 20px; background-color: white; }");

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

            hL->addSpacing(30);

            btn_exit_ = new IconButton(this, ":/editor.to-list");
            connect(btn_exit_, &IconButton::clicked, [this] { do_exit(); });
            btn_exit_->setBgColor("#29AC39");
            hL->addWidget(btn_exit_);

            btn_play_ = new IconButton(this, ":/cnc.play");
            connect(btn_play_, &IconButton::clicked, [this] { do_play();});
            btn_play_->setBgColor("#29AC39");
            hL->addWidget(btn_play_);

            hL->addSpacing(10);

            btn_save_ = new IconButton(this, ":/file.save");
            connect(btn_save_, &IconButton::clicked, [this] { do_save();});
            btn_save_->setBgColor("#29AC39");
            hL->addWidget(btn_save_);


            hL->addStretch();

            QList<QPair<QString, TableAc>> tableAcBtns = {
                { ":/editor.main-op",       AddMainOp     },
                { ":/editor.main-rel-op",   AddRelMainOp  },
                { ":/editor.pause",         AddPauseOp    },
                { ":/editor.inductor",      AddTimedFCOp  },
                { ":/editor.center",        AddCenterOp   },
                { ":/editor.go-to",         AddGoToOp     },
                { ":/editor.remove",        DeleteOp      }
            };

            for (auto const& ac : tableAcBtns)
            {
                IconButton *btn = new IconButton(this, ac.first, ac.second);
                btn->setBgColor("#29AC39");
                connect(btn, &IconButton::clickedId, [this](int op) { make_table_op(static_cast<TableAc>(op)); });
                hL->addWidget(btn);
            }
        }
        vL->addLayout(hL);

        comments_ = new ValueSetString(this);
        comments_->setValueAlignment(Qt::AlignLeft);
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

void editor_page_header_widget::showEvent(QShowEvent *)
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

    btn_exit_->setEnabled(!dirty);
    btn_play_->setEnabled(!dirty && !model_.empty());
}

void editor_page_header_widget::do_exit() noexcept
{
    if (btn_save_->isVisible())
    {
        msg_->show_question("Выйти без сохранения изменений?", [this]
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
    if (model_.name().isEmpty())
    {
        msg_->show_error("Не задано имя программы");
        return;
    }

    if (!model_.save_to_file())
    {
        msg_->show_error("Не удалось сохранить программу");
        return;
    }

    need_save(false);
}

void editor_page_header_widget::do_play() noexcept
{
    emit make_load();
}
