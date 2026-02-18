#include "ArcsWindow.h"

#include "ArcDataView.h"
#include "ArcsListWidget.h"
#include "ArcDataViewWidget.h"
// #include "arcs-list-page.h"

#include <Widgets/ScrollableTableView.h>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollBar>
#include <QScrollArea>
#include <QDateTime>

ArcsWindow::ArcsWindow(QWidget *parent)
    : QStackedWidget(parent)
    , BaseArcsLoader()
{
    arcsListW_ = new ArcsListWidget(this);
    QStackedWidget::addWidget(arcsListW_);

    arcDataViewW_ = new ArcDataViewWidget(this);
    connect(arcDataViewW_, &ArcDataViewWidget::go_back, [this] {
        QStackedWidget::setCurrentWidget(arcsListW_);    
    });
    QStackedWidget::addWidget(arcDataViewW_);

    connect(arcsListW_, &ArcsListWidget::open_archive, [this](std::size_t arc_id) 
    {
        open_selected_arc(arc_id);
    });
    
    connect(arcsListW_, &ArcsListWidget::update_page, [this] { 
        BaseArcsLoader::updatePages();
    });
}

void ArcsWindow::mounted(char const* const cpDest) noexcept
{
    // cpDest_ = cpDest;
    // btns_[BtnMakeArcsCopy]->setVisible(pages_->currentWidget() == tableView_);
}

void ArcsWindow::unmounted() noexcept
{
    // cpDest_.clear();
    // forCopy_.clear();
    // btns_[BtnMakeArcsCopy]->hide();
}

void ArcsWindow::open_selected_arc(std::size_t arcId)
{
    if (arcId == 0)
        return;
    
    // if (!forCopy_.empty() || !forRemove_.empty())
    //     return;

    //! Если мы снова выбрали предидущий загруженый архив, по новой ничего грузить не будем
    if (arcData_.arcId() == arcId)
    {
        QStackedWidget::setCurrentWidget(arcDataViewW_);
        return;
    }    
    
    arcData_.reset(arcId);
    inLoadArc_ = &arcData_;

    //! Проверяем наличие и необходимость загрузки базового архива
    // arcId = model_.getBaseArcId(seleced[0]);
    // if (arcBaseData_.arcId() != arcId)
    //     arcBaseData_.reset(arcId);

    //! Удаляем представление предидущего архива если таковое имеется
    // if (arcDataView_ != nullptr)
    // {
    //     scrollArea_->setWidget(nullptr);
    //     arcDataView_ = nullptr;
    // }
    //
    // QDateTime dt(QDateTime::fromString(model_.getBeginDate(seleced[0]), "dd-MM-yyyy HH:mm:ss"));
    // arcData_.beginDate = dt.toMSecsSinceEpoch();
    // arcData_.progName = model_.getProgName(seleced[0]).toUtf8().constData();
    //
    // BaseArcsLoader::loadArcData(inLoadArc_->arcId(), inLoadArc_->loadOffset());
}

//
// void ArcsWindow::onClose()
// {
//     // switchToPrev();
// }
//
// void ArcsWindow::onUpdateList()
// {
//     BaseArcsLoader::updateFilter();
// }
//
//
// void ArcsWindow::onToArcsList()
// {
//     pages_->setCurrentWidget(tableView_);
//
//     btns_[BtnClose]->show();
//     btns_[BtnListUpdate]->show();
//     btns_[BtnToArcsList]->hide();
//     btns_[BtnMakeBase]->hide();
//     btns_[BtnRemoveArcs]->show();
//     btns_[BtnMakeArcsCopy]->setVisible(!cpDest_.isEmpty());
//
//     // pbProgress_.hide();
// }

// void ArcsWindow::switchToArcView()
// {
    // pages_->setCurrentWidget(scrollArea_);

//     btns_[BtnClose]->hide();
//     btns_[BtnListUpdate]->hide();
//     btns_[BtnToArcsList]->show();
// //    btns_[BtnMakeBase]->show();
//     btns_[BtnMakeArcsCopy]->hide();
//     btns_[BtnRemoveArcs]->hide();
//
//     pbProgress_.hide();
// }

// void ArcsWindow::onMakeArcAsBase(bool asBase)
// {
//     // BaseArcsLoader::makeArcAsBase(arcData_.arcId(), asBase);
// }
//
//

void ArcsWindow::nfConnected()
{
    BaseArcsLoader::updateFilter();
}

void ArcsWindow::nfArcsCount(std::size_t arcsCount) 
{
    arcsListW_->model_.reset(arcsCount);
}

void ArcsWindow::nfLastArcId(std::size_t) 
{ }

std::size_t ArcsWindow::nfGetFirstRecordId()
{
    return arcsListW_->tableView_->rowAt(0);
}

std::size_t ArcsWindow::nfGetLastRecordId()
{
    int height = arcsListW_->tableView_->height();
    return arcsListW_->tableView_->rowAt(height);
}

std::size_t ArcsWindow::nfGetRowsInPage()
{
    return arcsListW_->model_.rowsInPage();
}

bool ArcsWindow::nfIsPageLoaded(std::size_t pageId)
{
    return arcsListW_->model_.isPageLoaded(pageId);
}

// void ArcsWindow::nfAppendNewArcData(std::size_t aId, nlohmann::json const &resp)
// {
//     // //! Если мы копируем архивы то
//     // if (!forCopy_.empty())
//     // {
//     //     auto& back = forCopy_.back();
//     //
//     //     //! Если получили не свои данные, игнорируем их
//     //     if (back.aId != aId)
//     //         return;
//     //
//     //     // TODO
//     //     std::size_t const inDataSize = 0;//resp.rawSize();
//     //
//     //     //! Если пришло 0 байт значит загрузка закончена, переходим к следующему
//     //     if (inDataSize == 0)
//     //     {
//     //         forCopy_.pop_back();
//     //         // TODO
//     //         // pbProgress_.setValue(pbProgress_.maximum() - forCopy_.size());
//     //
//     //         if (forCopy_.empty())
//     //         {
//     //             // TODO
//     //             // onToArcsList();
//     //             // SysDlg::inst().messageBoxDlg_->info("Копирование архивов закончено!");
//     //             return;
//     //         }
//     //     }
//     //     else
//     //     {
//     //         //! Если это первый блок данных архива,
//     //         //! открываем файл очищая его, иначе дописываем в конец
//     //
//     //         auto openmode = back.offset ? (std::ios::binary | std::ios::app) :
//     //                             (std::ios::binary | std::ios::trunc);
//     //         back.offset += inDataSize;
//     //
//     //         std::string fileName(cpDest_);
//     //         fileName += "/" + back.name + "(" + back.prog + ")";
//     //
//     //         //! Сохраняем данные в файл
//     //         std::ofstream file(fileName, openmode);
//     //         // file.write(reinterpret_cast<char const*>(resp.rawData()), inDataSize);
//     //     }
//     //
//     //     //! Ссылка на последний элемент может измениться
//     //     auto const& nback = forCopy_.back();
//     //     BaseArcsLoader::loadArcData(nback.aId, nback.offset);
//     //
//     //     return;
//     // }
//     //
//     // //! Игнорируем данные, пришедшие для другого архива
//     // //! такое может произойти если мы уже инициализированы для
//     // //! нового архива а загрузка предидущего еще не завершена
//     // if (inLoadArc_->arcId() != aId)
//     //     return;
//     //
//     // //! Если данная порция данных привела к инициализации
//     // bool inited = inLoadArc_->append(resp.rawData(), resp.rawSize());
//     //
//     // if (inited && (inLoadArc_ == &arcData_))
//     // {
//     //     arcDataView_ = new ArcDataView(arcData_, arcBaseData_);
//     //     scrollArea_->setWidget(arcDataView_);
//     //     switchToArcView();
//     // }
//     //
//     // if (inLoadArc_->shouldContinue())
//     // {
//     //     BaseArcsLoader::loadArcData(inLoadArc_->arcId(), inLoadArc_->loadOffset());
//     // }
//     // else
//     // {
//     //     //! Загрузка закончена!
//     //     //! Если архив так и не инициализирован сообщаем о ошибке загрузки архива
//     //     if (!inLoadArc_->arcInfo.inited())
//     //     {
//     //         if (inLoadArc_ == &arcBaseData_)
//     //             SysDlg::inst().messageBoxDlg_->error("Не удалось загрузить базовый архив!");
//     //         else
//     //             SysDlg::inst().messageBoxDlg_->error("Не удалось загрузить архив!");
//     //
//     //         inLoadArc_->reset(0);
//     //     }
//     //     //! Если мы грузили архив, переходим к загрузке базового архива
//     //     else if (inLoadArc_ == &arcData_ && arcBaseData_.needLoad())
//     //     {
//     //         inLoadArc_ = &arcBaseData_;
//     //         BaseArcsLoader::loadArcData(inLoadArc_->arcId(), inLoadArc_->loadOffset());
//     //     }
//     // }
//     //
//     // if (arcDataView_ != nullptr)
//     //     arcDataView_->newDataAppended();
// }

void ArcsWindow::nfArcWasRemoved(std::size_t)
{
    // if (forRemove_.empty())
    //     return;
    //
    // //! Если мы удаляем архивы */
    // forRemove_.pop_back();
    // pbProgress_.setValue(pbProgress_.maximum() - forRemove_.size());
    //
    // if (forRemove_.empty())
    // {
    //     onToArcsList();
    //     BaseArcsLoader::updateFilter();
    //     Interact::message(Interact::MsgInfo, "Архивы удалены!");
    // }
    // else
    // {
    //     BaseArcsLoader::removeArc(forRemove_.back());
    // }
}

// void ArcsWindow::nfAppendNewListData(std::size_t pageId, nlohmann::json const &args) 
// {
//     arcsListW_->model_.append(pageId, args);
// }
