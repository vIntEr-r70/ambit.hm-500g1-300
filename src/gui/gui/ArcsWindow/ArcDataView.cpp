#include "ArcDataView.h"

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QScrollBar>
#include <QStringList>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QGridLayout>
#include <QDateTime>

#include "DiagramView.h"
#include "ArcData.h"

template<typename Field, typename ... Args>
Field* createField(QGridLayout& grid, QString const& title, std::size_t rowId, Args&& ... args)
{
    Field* field = new Field(args ...);
    field->setStatusTip(title);
    grid.addWidget(new QLabel(title));
    grid.addWidget(field, rowId, 1);

    return field;
}

ArcDataView::ArcDataView(ArcData& ad, ArcData& abd)
    : QWidget()
    , arcData_(ad)
    , arcBaseData_(abd)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    {
        QHBoxLayout* hL = new QHBoxLayout();

        {
            QGridLayout* gL = new QGridLayout();
            gL->setHorizontalSpacing(20);

            std::size_t rowId = 0;

            arcProgName_ = createField<QLabel>(*gL, "Название процесса", rowId++);
            arcBeginDate_ = createField<QLabel>(*gL, "Начало процесса", rowId++);
            arcEndDate_ = createField<QLabel>(*gL, "Окончание процесса", rowId++);
            arcDuration_ = createField<QLabel>(*gL, "Длительность процесса", rowId++);

            hL->addLayout(gL);
        }

        hL->addStretch();
        hL->setStretch(1, 1);
        layout->addLayout(hL);
    }

    QDateTime timeBegin(QDateTime::fromMSecsSinceEpoch(arcData_.beginDate));
    arcBeginDate_->setText(timeBegin.toString("dd-MM-yyyy HH:mm:ss"));

    for (auto const& dgm : arcData_.arcInfo.dgms)
    {
//        dv_.push_back(new DiagramView(dgm, ad, abd, timeBegin.toString("dd-MM-yyyy HH:mm:ss").toUtf8().constData()));
        dv_.push_back(new DiagramView(dgm, ad, abd));
        layout->addWidget(dv_.back());
    }

    arcProgName_->setText(QString::fromStdString(arcData_.progName));
}

void ArcDataView::newDataAppended()
{
    for (auto& dv : dv_)
        dv->makeUpdate();

    //! Необходимо получить из файла первую и последную записи
    if (arcData_.rows() == 0)
        return;

    uint64_t keyMsTime = arcData_.asInt64(arcData_.rows() - 1, 0);

    QDateTime timeEnd(QDateTime::fromMSecsSinceEpoch(arcData_.beginDate + keyMsTime));
    arcEndDate_->setText(timeEnd.toString("dd-MM-yyyy HH:mm:ss"));

    unsigned sec = keyMsTime / 1000;

    char duration[64];
    std::snprintf(duration, sizeof(duration), "%02u:%02u:%02u", sec / 3600, (sec % 3600) / 60, sec % 60);
    arcDuration_->setText(duration);
}



