#include "programs-list-widget.hpp"

#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>

#include <Widgets/RoundButton.h>
#include <Widgets/VerticalScroll.h>
#include <InteractWidgets/MessageBox.h>

#include <filesystem>

#include <eng/log.hpp>

programs_list_widget::programs_list_widget(QWidget* parent)
    : QWidget(parent)
{
    auto LIAEM_RW_PATH = std::getenv("LIAEM_RW_PATH");
    path_ = LIAEM_RW_PATH ? LIAEM_RW_PATH : ".";
    path_ /= "programs";

    if (!std::filesystem::exists(path_))
        std::filesystem::create_directories(path_);

    QFont f(QWidget::font());
    f.setPointSize(16);

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    vL->setSpacing(30);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        hL->setSpacing(0); 
        {
            VerticalScroll *vs = new VerticalScroll(this);
            connect(vs, &VerticalScroll::move, [this](int shift) { make_scroll(shift); });

            local_list_ = new QTableWidget(this);

            connect(local_list_, &QTableWidget::itemSelectionChanged,
                    [this] { emit row_changed(); });
            connect(local_list_->verticalScrollBar(), &QScrollBar::rangeChanged,
                    [vs](int min, int max) { vs->change_range(min, max); });
            connect(local_list_->verticalScrollBar(), &QScrollBar::valueChanged,
                    [vs](int value) { vs->change_value(value); });
            local_list_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            local_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            local_list_->setFrameStyle(QFrame::Box);
            local_list_->setSelectionMode(QAbstractItemView::SingleSelection);
            local_list_->setSelectionBehavior(QAbstractItemView::SelectRows); 
            local_list_->setAlternatingRowColors(true);
            local_list_->setFont(f);
            local_list_->setColumnCount(2);
            local_list_->setColumnWidth(0, 400);
            local_list_->verticalHeader()->hide();
            local_list_->horizontalHeader()->setFont(f);
            local_list_->setSortingEnabled(true);
            local_list_->horizontalHeader()->setStretchLastSection(true);
            hL->addWidget(local_list_);

            hL->addSpacing(5);

            hL->addWidget(vs);
        }
        vL->addLayout(hL);
    }

    question_msg_box_ = new MessageBox(this, MessageBox::HeadQuestion);
}

QString programs_list_widget::current() const
{
    int row = local_list_->currentRow();
    if (row < 0) return "";

    QTableWidgetItem* item = local_list_->item(row, 0);
    return item->text();
}

void programs_list_widget::remove_selected()
{
    int row = local_list_->currentRow();
    if (row < 0) return;

    question_msg_box_->set_message(QString("Удалить программу %1?")
            .arg(local_list_->item(row, 0)->text()));
    question_msg_box_->set_buttons({ "Да", "Нет" });
    question_msg_box_->show([this, row](std::size_t ibtn)
    {
        question_msg_box_->hide();
        if (ibtn != 0) return;

        QString fname = local_list_->item(row, 0)->text();

        std::filesystem::path path{ path_ };
        path /= fname.toUtf8().constData();
        std::filesystem::remove(path);

        local_list_->removeRow(row);
    });
}

void programs_list_widget::make_scroll(int shift)
{
    auto bar = local_list_->verticalScrollBar();
    int pos = bar->sliderPosition();
    bar->setSliderPosition(pos + shift);
}

//! Необходимо загрузить список программ из базы
//! Так-же диалог должен позволять загружать программы из файловой системы

void programs_list_widget::showEvent(QShowEvent *)
{
    QString name;
    int row = local_list_->currentRow();
    if (row >= 0) name = local_list_->item(row, 0)->text();

    std::vector<std::pair<std::string, std::string>> files;
    for (auto const& dir_entry : std::filesystem::directory_iterator{path_})
    {
        if (!dir_entry.is_regular_file())
            continue;

        std::time_t cftime = std::chrono::system_clock::to_time_t(
                std::chrono::file_clock::to_sys(dir_entry.last_write_time()));

        std::tm *ptm = std::localtime(&cftime);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%d.%m.%Y", ptm);

        files.push_back({ dir_entry.path().filename(), buffer });
    }

    local_list_->clear();
    local_list_->setHorizontalHeaderLabels({ "Название", "Дата" });

    local_list_->setRowCount(files.size());

    local_list_->setSortingEnabled(false);
    for (std::size_t i = 0; i < files.size(); ++i)
    {
        QString fname(QString::fromLocal8Bit(files[i].first.c_str()));
        local_list_->setItem(i, 0, new QTableWidgetItem(fname));
        local_list_->setItem(i, 1, new QTableWidgetItem(files[i].second.c_str()));
        if (fname == name) local_list_->selectRow(i);
    }
    local_list_->setSortingEnabled(true);
}

