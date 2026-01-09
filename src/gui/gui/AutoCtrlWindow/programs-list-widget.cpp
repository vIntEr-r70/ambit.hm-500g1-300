#include "programs-list-widget.hpp"

#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>

#include <defs.h>

#include <Widgets/RoundButton.h>
#include <Widgets/VerticalScroll.h>

#include <aem/environment.h>

programs_list_widget::programs_list_widget(QWidget* parent)
    : QWidget(parent)
    , path_(aem::getenv("LIAEM_RW_PATH", "./home-root"))
{
    path_ /= "programs";

    if (!std::filesystem::exists(path_))
        std::filesystem::create_directories(path_);

    // setAttribute(Qt::WA_StyledBackground, true);
    // setStyleSheet("border-radius: 20px; background-color: #ffffff");
    //
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
            local_list_->setStyleSheet("border-radius: 0;");
            connect(local_list_, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onSelectInLocal(QModelIndex)));
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
            local_list_->setHorizontalHeaderLabels({ "Название", "Дата" });
            local_list_->setSortingEnabled(true);
            local_list_->horizontalHeader()->setStretchLastSection(true);
            hL->addWidget(local_list_);

            hL->addWidget(vs);
        }
        vL->addLayout(hL);

    //     hL = new QHBoxLayout();
    //     {
    //         btn_remove_ = new RoundButton(this);
    //         connect(btn_remove_, &RoundButton::clicked, [this] { make_delete_file(); });
    //         btn_remove_->setIcon(":/file.delete");
    //         btn_remove_->setBgColor("#e55056");
    //         hL->addWidget(btn_remove_);
    //
    //         hL->addStretch();
    //
    //         RoundButton *btn = new RoundButton(this);
    //         connect(btn, &RoundButton::clicked, [this] { make_load_file(); });
    //         btn->setText("Загрузить");
    //         btn->setBgColor("#29AC39");
    //         btn->setTextColor(Qt::white);
    //         btn->setMinimumWidth(100);
    //         hL->addWidget(btn);
    //
    //         btn = new RoundButton(this);
    //         connect(btn, &RoundButton::clicked, [this] { InteractWidget::hide(); });
    //         btn->setText("Закрыть");
    //         btn->setBgColor("#E55056");
    //         btn->setTextColor(Qt::white);
    //         btn->setMinimumWidth(100);
    //         hL->addWidget(btn);
    //     }
    //     vL->addLayout(hL);
    }
}

// void ProgramListDlg::set_guid(int guid)
// {
//     btn_remove_->setVisible(guid != auth_user);
// }

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

    local_list_->setRowCount(files.size());
    for (std::size_t i = 0; i < files.size(); ++i)
    {
        local_list_->setItem(i, 0, new QTableWidgetItem(files[i].first.c_str()));
        local_list_->setItem(i, 1, new QTableWidgetItem(files[i].second.c_str()));
    }
}

void programs_list_widget::onSelectInLocal(QModelIndex index)
{
    if (!index.isValid())
        return;
    make_load_file();
}

void programs_list_widget::make_load_file()
{
    // int row = local_list_->currentRow();
    // if (row < 0) return;
    //
    // QTableWidgetItem* item = local_list_->item(row, 0);
    // InteractWidget::hide();
    //
    // emit makeLoadFromLocalFile(item->text());
}

// void ProgramListDlg::make_delete_file()
// {
//     int row = local_list_->currentRow();
//     if (row < 0) return;
//
//     QTableWidgetItem* item = local_list_->item(row, 0);
//     std::string fname(item->text().toUtf8().constData());
//
//     try 
//     {
//         std::filesystem::remove(path_ / fname);
//         local_list_->removeRow(row);
//     }
//     catch(...) 
//     { 
//     }
// }

