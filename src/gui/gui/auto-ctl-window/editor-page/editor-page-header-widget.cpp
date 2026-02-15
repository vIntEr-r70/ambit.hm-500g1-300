#include "editor-page-header-widget.hpp"

#include <Widgets/ValueSetString.h>
#include <Widgets/IconButton.h>

#include <QVBoxLayout>
#include <QTableView>
#include <QGraphicsDropShadowEffect>

editor_page_header_widget::editor_page_header_widget(QWidget *parent) noexcept
    : QWidget(parent)
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
            connect(name_, &ValueSetString::onValueChanged, [this] {
                need_save(true); 
            });
            name_->setTitle("Название программы");
            name_->setMaximumWidth(400);
            name_->setMinimumWidth(400);
            hL->addWidget(name_);

            hL->addSpacing(30);

            btn_exit_ = new IconButton(this, ":/editor.to-list");
            connect(btn_exit_, &IconButton::clicked, [this] { emit do_exit(); });
            btn_exit_->setBgColor("#29AC39");
            hL->addWidget(btn_exit_);

            btn_play_ = new IconButton(this, ":/cnc.play");
            connect(btn_play_, &IconButton::clicked, [this] { emit do_play(); });
            btn_play_->setBgColor("#29AC39");
            hL->addWidget(btn_play_);

            hL->addSpacing(10);

            btn_save_ = new IconButton(this, ":/file.save");
            connect(btn_save_, &IconButton::clicked, [this] { emit do_save(); });
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
                btn->setBgColor(ac.second != DeleteOp ? "#29AC39" : "#E55056");
                connect(btn, &IconButton::clickedId, [this](int op) {
                        emit make_table_op(static_cast<TableAc>(op));
                    });
                hL->addWidget(btn);
            }
        }
        vL->addLayout(hL);

        comments_ = new ValueSetString(this);
        comments_->setValueAlignment(Qt::AlignLeft);
        connect(comments_, &ValueSetString::onValueChanged, [this] {
            need_save(true);
        });
        comments_->setTitle("Коментарии");
        vL->addWidget(comments_);
    }
}

void editor_page_header_widget::set_program_info(QString const &name, QString const &comments)
{
    name_->set_value(name);
    comments_->set_value(comments);
}

void editor_page_header_widget::need_save(bool dirty)
{
    btn_save_->setVisible(dirty);
    btn_exit_->setEnabled(!dirty);
    btn_play_->setEnabled(!dirty);
}

bool editor_page_header_widget::need_save() const noexcept
{
    return btn_save_->isVisible();
}

QString const &editor_page_header_widget::name() const noexcept
{
    return name_->value();
}

QString const &editor_page_header_widget::comments() const noexcept
{
    return comments_->value();
}
