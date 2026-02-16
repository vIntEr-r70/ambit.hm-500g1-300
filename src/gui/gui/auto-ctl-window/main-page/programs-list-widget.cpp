#include "programs-list-widget.hpp"
#include "program-list-model.hpp"
#include "main-page-header-widget.hpp"
#include "../common/program-record.hpp"

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QScrollBar>
#include <QStyledItemDelegate>

#include <Widgets/RoundButton.h>
#include <Widgets/VerticalScroll.h>
#include <InteractWidgets/MessageBox.h>

#include <eng/log.hpp>

programs_list_widget::programs_list_widget(QWidget* parent, main_page_header_widget *header)
    : QWidget(parent)
    , header_(header)
{
    auto LIAEM_RW_PATH = std::getenv("LIAEM_RW_PATH");
    std::filesystem::path path = LIAEM_RW_PATH ? LIAEM_RW_PATH : ".";
    path /= "programs";

    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);

    model_ = new program_list_model();
    model_->add_local_path(std::move(path));
    model_->add_usb_path("/mnt");

    question_msg_box_ = new MessageBox(this, MessageBox::HeadQuestion);

    QFont f(QWidget::font());
    f.setPointSize(16);

    QVBoxLayout* vL = new QVBoxLayout(this);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        hL->setSpacing(0); 
        {
            VerticalScroll *vs = new VerticalScroll(this);
            connect(vs, &VerticalScroll::move, [this](int shift) { make_scroll(shift); });

            list_ = new QTableView(this);
            list_->setModel(model_);

            connect(list_->selectionModel(), &QItemSelectionModel::currentRowChanged,
                    [this] {
                        update_selection();
                    });

            connect(list_->verticalScrollBar(), &QScrollBar::rangeChanged,
                    [vs](int min, int max) { vs->change_range(min, max); });
            connect(list_->verticalScrollBar(), &QScrollBar::valueChanged,
                    [vs](int value) { vs->change_value(value); });

            list_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            list_->setFocusPolicy(Qt::NoFocus);
            list_->setFrameStyle(QFrame::Box);
            list_->setSelectionMode(QAbstractItemView::SingleSelection);
            list_->setSelectionBehavior(QAbstractItemView::SelectRows);
            list_->setAlternatingRowColors(true);
            list_->setFont(f);
            list_->setColumnWidth(0, 400);
            list_->setColumnWidth(1, 170);
            list_->setColumnWidth(2, 0);
            list_->setColumnWidth(3, 0);
            list_->verticalHeader()->hide();
            list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            list_->horizontalHeader()->setFont(f);
            list_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            list_->setSortingEnabled(true);
            hL->addWidget(list_);

            hL->addSpacing(5);

            hL->addWidget(vs);
        }
        vL->addLayout(hL);

        vL->addSpacing(10);

        hL = new QHBoxLayout();
        hL->setSpacing(0);
        {
            btn_to_hd_ = new RoundButton(this);
            btn_to_hd_->setIcon(":/file.folder-open");
            connect(btn_to_hd_, &RoundButton::clicked, [this] { copy_to_hd(); });
            hL->addWidget(btn_to_hd_);

            QLabel *lbl = new QLabel(this);
            lbl->setPixmap(QPixmap(":/file.sync"));
            hL->addWidget(lbl);

            btn_to_usb_ = new RoundButton(this);
            btn_to_usb_->setIcon(":/file.usb");
            connect(btn_to_usb_, &RoundButton::clicked, [this] { copy_to_usb(); });
            hL->addWidget(btn_to_usb_);

            hL->addStretch();
        }
        vL->addLayout(hL);
    }

    update_selection();
}

void programs_list_widget::update_selection()
{
    QModelIndex index = list_->currentIndex();

    if (index.isValid())
    {
        program_record_t const *r = model_->record(index.row());

        auto const &fn = r->filename;
        auto const &comm = r->comments;

        header_->set_program_info(
                QString::fromUtf8(fn.data(), fn.length()),
                QString::fromUtf8(comm.data(), comm.length()));

        btn_to_hd_->setEnabled(!r->source.test(program_record_t::hd_bit) && !r->syncing);
        btn_to_usb_->setEnabled(!r->source.test(program_record_t::usb_bit) && !r->syncing);
    }
    else
    {
        btn_to_hd_->setEnabled(false);
        btn_to_usb_->setEnabled(false);

        header_->set_program_info("", "");
    }
}

void programs_list_widget::copy_to_hd()
{
    btn_to_hd_->setEnabled(false);
    model_->copy_to_hd(list_->currentIndex().row());
}

void programs_list_widget::copy_to_usb()
{
    btn_to_usb_->setEnabled(false);
    model_->copy_to_usb(list_->currentIndex().row());
}

program_record_t const *programs_list_widget::selected_program() const
{
    QModelIndex index = list_->currentIndex();
    return index.isValid() ? model_->record(index.row()) : nullptr;
}

program_record_t const *programs_list_widget::find_program_by_name(std::string const &fname) const
{
    return model_->record(fname);
}

void programs_list_widget::remove_selected()
{
    model_->remove(list_->currentIndex().row());
    list_->setCurrentIndex(QModelIndex{});

    // int row = local_list_->currentRow();
    // if (row < 0) return;
    //
    // question_msg_box_->set_message(QString("Удалить программу %1?")
    //         .arg(local_list_->item(row, 0)->text()));
    // question_msg_box_->set_buttons({ "Да", "Нет" });
    // question_msg_box_->show([this, row](std::size_t ibtn)
    // {
    //     question_msg_box_->hide();
    //     if (ibtn != 0) return;
    //
    //     QString fname = local_list_->item(row, 0)->text();
    //
    //     std::filesystem::path path{ path_ };
    //     path /= fname.toUtf8().constData();
    //     std::filesystem::remove(path);
    //
    //     local_list_->removeRow(row);
    // });
}

void programs_list_widget::make_scroll(int shift)
{
    auto bar = list_->verticalScrollBar();
    int pos = bar->sliderPosition();
    bar->setSliderPosition(pos + shift);
}

void programs_list_widget::showEvent(QShowEvent *)
{
}

