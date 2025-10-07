#include "ArcsListWidget.h"

#include <Widgets/RoundButton.h>
#include <Widgets/ScrollableTableView.h>

#include <QHeaderView>
#include <QVBoxLayout>
#include <QScrollBar>

ArcsListWidget::ArcsListWidget(QWidget *parent) noexcept
    : QWidget(parent)
    , model_(100)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        QHBoxLayout* hL = new QHBoxLayout();
        {
            hL->addStretch();

            btns_[BtnOpenArc] = new RoundButton(this);
            btns_[BtnOpenArc]->setText("Открыть");
            connect(btns_[BtnOpenArc], &RoundButton::clicked, [this] { emit open_archive(0); });

            // btns_[BtnMakeBase] = new QPushButton("ИСПОЛЬЗОВАТЬ КАК ЭТАЛОН", this);
            // btns_[BtnMakeBase]->setCheckable(true);
            // connect(btns_[BtnMakeBase], SIGNAL(clicked(bool)), this, SLOT(onMakeArcAsBase(bool)));

            // btns_[BtnToArcsList] = new QPushButton("ВЕРНУТЬСЯ К СПИСКУ АРХИВОВ", this);
            // connect(btns_[BtnToArcsList], SIGNAL(clicked(bool)), this, SLOT(onToArcsList()));

            // btns_[BtnRemoveArcs] = new QPushButton("УДАЛИТЬ ВЫБРАННЫЕ", this);
            // connect(btns_[BtnRemoveArcs], SIGNAL(clicked(bool)), this, SLOT(onRemoveSelectedArcs()));

            // btns_[BtnMakeArcsCopy] = new QPushButton("КОПИРОВАТЬ ВЫБРАННЫЕ", this);
            // connect(btns_[BtnMakeArcsCopy], SIGNAL(clicked(bool)), this, SLOT(onCopySelectedArcs()));

            // btns_[BtnListUpdate] = new QPushButton("ОБНОВИТЬ СПИСОК", this);
            // connect(btns_[BtnListUpdate], SIGNAL(clicked(bool)), this, SLOT(onUpdateList()));

            // btns_[BtnClose] = new QPushButton("ЗАКРЫТЬ", this);
            // connect(btns_[BtnClose], SIGNAL(clicked(bool)), this, SLOT(onClose()));

            for (auto btn : btns_)
                hL->addWidget(btn);
        }
        vL->addLayout(hL);

        tableView_ = new ScrollableTableView(this);
        tableView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
        connect(tableView_, &QTableView::doubleClicked, [this](QModelIndex const &index) {
            emit open_archive(model_.getArcId(index));
        });
        connect(tableView_->verticalScrollBar(), &QScrollBar::valueChanged, [this](int) {
            emit update_page();                
        });
    
        tableView_->setModel(&model_);

        tableView_->setColumnWidth(0, 170);
        tableView_->setColumnWidth(1, 250);
        tableView_->setColumnWidth(2, 100);

        vL->addWidget(tableView_);
    }
}

// void ArcsWindow::onCopySelectedArcs()
// {
//     //! Если мы что-то удаляем то копировать нельзя
//     if (!forRemove_.empty())
//         return;
//
//     //! Формируем список идентификаторов
//     QModelIndexList selected(tableView_->selectionModel()->selectedRows());
//     if (selected.empty())
//         return;
//
//     forCopy_.clear();
//
//     for (int i = 0; i < selected.size(); ++i)
//     {
//         std::size_t arcId = model_.getArcId(selected[i]);
//
//         QDateTime dt(QDateTime::fromString(model_.getBeginDate(selected[i]), "dd-MM-yyyy HH:mm:ss"));
//         std::string fn(dt.toString("dd-MM-yyyy_HH-mm-ss").toStdString());
//         std::string pn(model_.getProgName(selected[i]).toStdString());
//         forCopy_.push_back({ arcId, 0, fn, pn });
//     }
//
//     //! Запускаем процесс загрузки каждого записывая их в файл
//     //! И так для каждого, показывая текущий копируемый
//     auto const& back = forCopy_.back();
//     BaseArcsLoader::loadArcData(back.aId, back.offset);
//
//     // pbProgress_.setRange(0, forCopy_.size());
//     // pbProgress_.setValue(0);
//     // pbProgress_.show();
// }
//
// void ArcsWindow::onRemoveSelectedArcs()
// {
//     // //! Формируем список идентификаторов
//     // QModelIndexList selected(tableView_->selectionModel()->selectedRows());
//     // if (selected.empty())
//     //     return;
//     //
//     // QString msg("Будут удалены ");
//     // msg += QString::number(selected.size()) + " архивов! Продолжить?";
//     //
//     // Interact::question(msg, [this](bool) 
//     // {
//     //     //! Формируем список идентификаторов
//     //     QModelIndexList selected(tableView_->selectionModel()->selectedRows());
//     //     if (selected.empty())
//     //         return;
//     //
//     //     for (int i = 0; i < selected.size(); ++i)
//     //     {
//     //         std::size_t arcId = model_.getArcId(selected[i]);
//     //         forRemove_.push_back(arcId);
//     //     }
//     //
//     //     BaseArcsLoader::removeArc(forRemove_.back());
//     //
//     //     pbProgress_.setRange(0, forRemove_.size());
//     //     pbProgress_.setValue(0);
//     //     pbProgress_.show();
//     // });
// }
// 
// 
