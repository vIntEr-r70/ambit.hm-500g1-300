#include "arcs-list-page.h"
/* #include "../main-header.h" */
#include "arcs-list-model.h"

/* #include "common-gui/output.h" */
/* #include "common-gui/header.h" */

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>

#include <aem/log.h>

arcs_list_page::arcs_list_page(QWidget *parent) noexcept
    : QFrame(parent)
    /* , header_(wnd_header) */
{
    setFrameStyle(QFrame::StyledPanel);
    model_ = new arcs_list_model();

    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        table_ = new QTableView(this);
        table_->setModel(model_);
        connect(table_->selectionModel(), SIGNAL(currentRowChanged(QModelIndex const&, QModelIndex const&)), 
                this, SLOT(on_row_changed(QModelIndex const&, QModelIndex const&)));

        QScrollBar* verticalScroll =  table_->verticalScrollBar();
        connect(verticalScroll, SIGNAL(valueChanged(int)), this, SLOT(onVerticalValueChanged(int)));       

        table_->setFocusPolicy(Qt::NoFocus);
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::SingleSelection);
	    table_->horizontalHeader()->setStretchLastSection(true);
	    table_->horizontalHeader()->resizeSection(0, 160);
	    table_->horizontalHeader()->resizeSection(1, 80);
	    table_->horizontalHeader()->resizeSection(2, 150);
	    table_->horizontalHeader()->resizeSection(3, 300);
	    table_->horizontalHeader()->resizeSection(4, 100);
	    table_->horizontalHeader()->resizeSection(5, 320);
	    table_->horizontalHeader()->resizeSection(6, 100);
	    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	    table_->verticalHeader()->setDefaultSectionSize(30);
	    table_->verticalHeader()->hide();
	    table_->setAlternatingRowColors(true);
        vL->addWidget(table_);

        /* QHBoxLayout *hL = new QHBoxLayout(); */
        /* { */
        /*     QVBoxLayout *vL = new QVBoxLayout(); */
        /*     { */
        /*         QHBoxLayout *hL = new QHBoxLayout(); */
        /*         { */
        /*             QPushButton *btn = new QPushButton(this); */
        /*             connect(btn, &QPushButton::clicked, [this] { on_open_arc(); }); */
        /*             btn->setText("Открыть"); */
        /*             hL->addWidget(btn); */
        /*  */
        /*             btn = new QPushButton(this); */
        /*             connect(btn, &QPushButton::clicked, [this]  */
        /*             {  */
        /*                 if (irow_ != -1) */
        /*                 { */
        /*                     std::size_t aid = model_->get_arc_id(irow_); */
        /*                     if (aid != 0) emit do_remove(aid);  */
        /*                 } */
        /*             }); */
        /*             btn->setText("Удалить"); */
        /*             hL->addWidget(btn); */
        /*              */
        /*             btn = new QPushButton(this); */
        /*             connect(btn, &QPushButton::clicked, [this] { emit do_export(); }); */
        /*             btn->setText("Экспортировать"); */
        /*             hL->addWidget(btn); */
        /*  */
        /*             btn = new QPushButton(this); */
        /*             connect(btn, &QPushButton::clicked, [this] { emit do_copy(); }); */
        /*             btn->setText("Копировать"); */
        /*             hL->addWidget(btn); */
        /*         } */
        /*         vL->addLayout(hL); */
        /*  */
        /*         vL->addStretch(); */
        /*  */
        /*         hL = new QHBoxLayout(); */
        /*         { */
        /*             QVBoxLayout *vL = new QVBoxLayout(); */
        /*             vL->setSpacing(2); */
        /*             { */
        /*                 vL->addWidget(new header(this, "Имя ЦГ")); */
        /*                 cg_name_ = new QLabel(this); */
        /*                 cg_name_->setFrameStyle(QFrame::StyledPanel); */
        /*                 cg_name_->setAlignment(Qt::AlignCenter); */
        /*                 vL->addWidget(cg_name_); */
        /*             } */
        /*             hL->addLayout(vL); */
        /*          */
        /*             vL = new QVBoxLayout(); */
        /*             vL->setSpacing(2); */
        /*             { */
        /*                 vL->addWidget(new header(this, "Тип ЦГ")); */
        /*                 cg_type_ = new QLabel(this); */
        /*                 cg_type_->setFrameStyle(QFrame::StyledPanel); */
        /*                 cg_type_->setAlignment(Qt::AlignCenter); */
        /*                 vL->addWidget(cg_type_); */
        /*             } */
        /*             hL->addLayout(vL); */
        /*         } */
        /*         vL->addLayout(hL); */
        /*  */
        /*         vL->addStretch(); */
        /*     } */
        /*     hL->addLayout(vL); */
        /*  */
        /*     hL->addSpacing(50); */
        /*  */
        /*     vL = new QVBoxLayout(); */
        /*     vL->setSpacing(2); */
        /*     { */
        /*         vL->addWidget(new header(this, "Назначение работы")); */
        /*      */
        /*         work_desc_ = new QTextEdit(this); */
        /*         work_desc_->setReadOnly(true); */
        /*         work_desc_->setMinimumWidth(400); */
        /*         work_desc_->setMaximumHeight(80); */
        /*         vL->addWidget(work_desc_); */
        /*     } */
        /*     hL->addLayout(vL); */
        /*  */
        /*     hL->setStretch(0, 1); */
        /* } */
        /* vL->addLayout(hL); */
    }

    /* vL->setStretch(0, 1); */
}

void arcs_list_page::close_model() noexcept
{
    model_->close_model();
}

void arcs_list_page::on_row_changed(QModelIndex const& index, QModelIndex const&) noexcept
{
    irow_ = index.isValid() ? index.row() : -1;
    aem::log::info("arcs_list_page::on_row_changed: {}", irow_);

    if (irow_ == -1)
    {
        work_desc_->clear();
        cg_name_->setText("");
        cg_type_->setText("");

        return;
    }

    work_desc_->setPlainText(model_->get_work_desc(irow_));
    cg_name_->setText(model_->get_cg_name(irow_));
    cg_type_->setText(model_->get_cg_type(irow_));
}

void arcs_list_page::on_open_arc() noexcept
{
    if (irow_ == -1)
        return;

    std::size_t aid = model_->get_arc_id(irow_);
    aem::log::info("arcs_list_page::on_open_arc: {}", aid);
    if (aid == 0)
        return;

    /* header_.set_ab_num(model_->get_ab_num(irow_)); */
    /* header_.set_zrk_num(model_->get_zrk_num(irow_)); */
    /* header_.set_user_name(model_->get_user_name(irow_)); */
    /* header_.set_arc_name(model_->get_arc_name(irow_)); */

    emit load_arc(aid);
}

void arcs_list_page::set_cg_mode(char const* mode) noexcept 
{
    cg_type_->setText(mode);

}
void arcs_list_page::onVerticalValueChanged(int rowId) noexcept
{
    int dnRow = table_->rowAt(table_->viewport()->height());
    if (dnRow < 0) dnRow = rowId;
    model_->loadPage(rowId, dnRow);
}
