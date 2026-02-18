#include "BaseArcsLoader.h"

BaseArcsLoader::BaseArcsLoader()
{
    // arcsSrc_.connect(global::arcs_db_host(), global::arcs_db_port());
    // arcsSrc_.on_connected = [this] { nfConnected(); };
}

void BaseArcsLoader::makeArcAsBase(std::size_t arcId, bool asBase)
{
    // arcsSrc_.reset();
    // arcsSrc_.add(arcId);
    // arcsSrc_.add(asBase);
    // arcsSrc_.call(uArcs::markArcAsBase);
}

void BaseArcsLoader::saveProgramCtrl(std::size_t const progId, std::string const& ctrl)
{
    // arcsSrc_.call("update-ctrl", { progId, ctrl })
    //     .done([this](nlohmann::json const &args)
    //     { 
    //     });
}

//void BaseArcsLoader::saveProgramVerifyFlag(std::size_t const progId, bool const verify)
//{
//    arcsSrc_.reset();
//    arcsSrc_.add(progId);
//    arcsSrc_.add(verify);
//    arcsSrc_.call(uArcs::prog_updateVerify);
//}

void BaseArcsLoader::updateFilter()
{
    //! Идентификатор прошлого запроса
    //! Порядковый номер дня
    //! Порядковый номер месяца
    //! Порядковый номер года
    //! Часть имени пользователя
    //! Часть названия программы

    // arcsSrc_.call("init-list", { stmtId_, 0, 0, 0, "", "" })
    //     .done([this](nlohmann::json const &args)
    //     { 
    //         stmtId_ = args[0].get<std::uint64_t>();
    //         arcsCount_ = args[1].get<std::size_t>();
    //
    //         aem::log::trace("init-list: {}, {}", stmtId_, arcsCount_);
    //
    //         nfArcsCount(arcsCount_);
    //
    //         if (arcsCount_ != 0)
    //             updatePages();
    //     })
    //     .error([this](std::string_view emsg)
    //     {
    //         aem::log::error("init-list: {}", emsg);
    //     }); 
}

void BaseArcsLoader::updateLastArcId()
{
    // arcsSrc_.call("get-last-id", { })
    //     .done([this](nlohmann::json const &args)
    //     { 
    //         std::size_t lastArcId = args[0].get<std::size_t>();
    //         if (lastArcId == lastArcId_)
    //             return;
    //         lastArcId_ = lastArcId;
    //         nfLastArcId(lastArcId_);
    //     });
}

void BaseArcsLoader::loadArcData(std::size_t const arcId, std::size_t const from)
{
    // arcsSrc_.call("load-data", { arcId, from })
    //     .done([this](nlohmann::json const &args)
    //     { 
    //         std::size_t aId = args[0].get<std::size_t>();
    //         nfAppendNewArcData(aId, args[1]);
    //     });
}

void BaseArcsLoader::removeArc(std::size_t arcId)
{
    // arcsSrc_.call("remove", { arcId })
    //     .done([this](nlohmann::json const &args)
    //     { 
    //         std::size_t arcId = args[0].get<std::size_t>();
    //         nfArcWasRemoved(arcId);
    //     });
}

void BaseArcsLoader::updatePages()
{
    // if (arcsCount_ == 0)
    //     return;
    //
    // std::size_t rowsId[2];
    // rowsId[0] = nfGetFirstRecordId();
    // rowsId[1] = nfGetLastRecordId();
    //
    // for (std::size_t i = 0; i < 2; ++i)
    // {
    //     if (rowsId[i] >= arcsCount_)
    //         rowsId[i] = arcsCount_ - 1;
    //
    //     std::size_t pageId = rowsId[i] / nfGetRowsInPage();
    //
    //     //! Если страница уже загружена, ее данные будут отображены при скролировании
    //     if (nfIsPageLoaded(pageId))
    //         continue;
    //
    //     //! Если страница уже находится в очереди,
    //     //! нет необходимости ее туда добавлять еще раз
    //     auto it = std::find(pagesQueue_.begin(), pagesQueue_.end(), pageId);
    //     if (it != pagesQueue_.end())
    //         continue;
    //
    //     //! Сохраняем страницу в очереди на загрузку
    //     pagesQueue_.push_back(pageId);
    //
    //     //! Если в данный момент уже происходить загрузка страницы, выходим
    //     if (pagesQueue_.size() > 1)
    //         continue;
    //
    //     //! Инициируем загрузку страницы
    //     initLoadPage(pageId);
    // }
}

void BaseArcsLoader::initLoadPage(std::size_t const pageId)
{
    //! Идентификатор запроса для данного клиента
    //! Порядковый номер запрашиваемой записи
    //! Количество ожидаемых записей

    //! Необходимо загрузить страницу
    // arcsSrc_.call("load-list", { stmtId_, pageId * nfGetRowsInPage(), nfGetRowsInPage() })
    //     .done([this](nlohmann::json const &args)
    //     { 
    //         //! Удаляем из очереди текущую загруженную страницу
    //         std::size_t pageId = pagesQueue_.front();
    //         pagesQueue_.pop_front();
    //
    //         nfAppendNewListData(pageId, args);
    //
    //         //! Если в очереди накопились страницы,
    //         //! удаляем их все загружая только последную
    //         while (pagesQueue_.size() > 1)
    //             pagesQueue_.pop_front();
    //
    //         if (!pagesQueue_.empty())
    //             initLoadPage(pagesQueue_.front());
    //     });
}


