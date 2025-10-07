#pragma once 

#include "ArcsListModel.h"

#include <QWidget>

class ScrollableTableView;
class RoundButton;

class ArcsListWidget final
    : public QWidget
{
    Q_OBJECT

    enum EditBtns
    {
        BtnOpenArc,
        // BtnMakeArcsCopy,
        // BtnRemoveArcs,
        // BtnListUpdate,
        // BtnClose,
        // BtnMakeBase,
        // BtnToArcsList,

        BtnsCount
    };

public:

    ArcsListWidget(QWidget*) noexcept;

signals:

    void open_archive(std::size_t);

    void update_page();

public:
    
    ArcsListModel model_;
    ScrollableTableView* tableView_;
    
private:

    std::array<RoundButton*, BtnsCount> btns_;

    struct CopyDesc
    {
        std::size_t aId;
        std::size_t offset;
        std::string name;
        std::string prog;
    };
    std::vector<CopyDesc> forCopy_;
    std::string cpDest_;

    std::vector<std::size_t> forRemove_;
};
