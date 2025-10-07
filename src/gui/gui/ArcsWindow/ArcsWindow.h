#pragma once

#include "BaseArcsLoader.h"

// #include <map>
// #include <queue>

// #include <QProgressBar>
#include <QStackedWidget>
#include <qobjectdefs.h>

#include "ArcData.h"

class ArcsListWidget;
// class FileSystemWidget;
// class QLabel;
class QScrollArea;
class QStackedWidget;
class ArcDataView;

class ArcDataViewWidget;
class arcs_list_page;


class ArcsWindow
    : public QStackedWidget 
    , public BaseArcsLoader
{
public:

    ArcsWindow(QWidget*);

public:

    void mounted(char const*) noexcept;

    void unmounted() noexcept;

private:

    void nfConnected() override final;
    
    void nfArcsCount(std::size_t) override final;
        
    void nfLastArcId(std::size_t) override final;
    
    std::size_t nfGetFirstRecordId() override final;
    
    std::size_t nfGetLastRecordId() override final;
    
    std::size_t nfGetRowsInPage() override final;
    
    bool nfIsPageLoaded(std::size_t) override final;

    void nfAppendNewArcData(std::size_t, nlohmann::json const &) override final;

    void nfAppendNewListData(std::size_t, nlohmann::json const &) override final;

    void nfArcWasRemoved(std::size_t) override final;

private slots:

    // void onClose();
    //
    // void onToArcsList();
    //
    // void onUpdateList();
    //
    // void onMakeArcAsBase(bool);
    //

private:
    
    void open_selected_arc(std::size_t);

    void initLoadPage(std::size_t);

    void switchToArcView();

    void openSelectedArc();

private:

    ArcsListWidget *arcsListW_;
    // arcs_list_page *arcsListW_;
    ArcDataViewWidget *arcDataViewW_;

    // QScrollArea* scrollArea_;
    // QStackedWidget* pages_;

    // QProgressBar pbProgress_;
    // QString cpDest_;

    ArcDataView* arcDataView_{ nullptr };

    ArcData arcData_;
    // ArcData arcBaseData_;
    ArcData* inLoadArc_{ nullptr };
};

