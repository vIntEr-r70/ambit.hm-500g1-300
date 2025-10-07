#include "DiagramView.h"

#include <QVBoxLayout>
#include <QLabel>

//!Порядок графиков:
//!1 Мощность
//!2 Ток
//!3 Спреер
//!4 Координата "Z"
//!5 Координата "X"
//!6 Координата "Y"
//!7 Координата "U"
//!8 Датчик температуры 1
//!9 Датчик температуры 2


DiagramView::DiagramView(ArcInfo::DgmDesc const& dgm, ArcData const& ad, ArcData const& abd)
    : QWidget()
    , arcData_(ad)
    , arcBaseData_(abd)
    , cId_(dgm.cId)
{
    setFixedHeight(200);
    setAttribute(Qt::WA_NoMousePropagation, false);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    {
        QHBoxLayout* hL = new QHBoxLayout();
        hL->addWidget(new QLabel(QString::fromUtf8(dgm.title.c_str()), this));
        hL->addStretch();
        lblErr_ = new QLabel("- - -", this);
        hL->addWidget(lblErr_);
        layout->addLayout(hL);
    }
    layout->addWidget(&plot_);
    layout->setStretch(1, 1);

    QFont font;
    font.setPointSize(10);

    QCPGraph* g = plot_.addGraph(plot_.xAxis, plot_.yAxis);
    g->setPen(QPen(Qt::blue));
    g->setLineStyle(dgm.asFlags ?  QCPGraph::lsStepCenter : QCPGraph::lsLine);

    //! Добавляем второй график
    if (abd.arcId() != 0)
    {
        g = plot_.addGraph(plot_.xAxis, plot_.yAxis);
        g->setPen(QPen(Qt::red));
        g->setLineStyle(dgm.asFlags ?  QCPGraph::lsStepCenter : QCPGraph::lsLine);
    }

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    plot_.xAxis->setTicker(timeTicker);
    plot_.xAxis->setTickLabelFont(font);
    plot_.yAxis->setTickLabelFont(font);

    plot_.replot();
}

void DiagramView::makeUpdate()
{
    //! Выводим данные основного архива
    if (rows_ != arcData_.rows())
    {
        double key = 0;

        QCPGraph* g = plot_.graph(0);
        for (std::size_t rId = rows_; rId < arcData_.rows(); ++rId)
        {
            double val = arcData_.asDouble(rId, cId_);
            min_ = std::min(min_, val);
            max_ = std::max(max_, val);

            key = arcData_.asDouble(rId, 0) / 1000;
            g->addData(key, val);
       }

        plot_.xAxis->setRange(0, key * 1.01);
        plot_.yAxis->setRange(min_ - 1, max_ + 1);

        rows_ = arcData_.rows();
    }

    //! Выводим данные базового архива, если таковой имеется
    if (baseRows_ != arcBaseData_.rows())
    {
        QCPGraph* g = plot_.graph(1);
        for (std::size_t rId = baseRows_; rId < arcBaseData_.rows(); ++rId)
        {
            double val = arcBaseData_.asDouble(rId, cId_);
            double key = arcBaseData_.asDouble(rId, 0) / 1000;
            g->addData(key, val);
        }

        baseRows_ = arcBaseData_.rows();
    }

    appendError();

    plot_.replot();
}

void DiagramView::appendError() noexcept
{
    //! Ошибка считается только если графиков два
    if (plot_.graphCount() < 2)
        return;

    QCPGraph* gBase = plot_.graph(1);
    QCPGraph* gMeas = plot_.graph(0);

/*    QCPGraph* g1 = plot_.graph(2);
    g1->data().clear();

    QCPGraph* g2 = plot_.graph(3);
    g2->data().clear();*/


    //! Получаем количество точек, которые уже загрузились в обоих графиках
    int const minN = std::min(gBase->dataCount(), gMeas->dataCount());

    //! Если какой-либо из графиков еще не содержит точек, выходим
    if (minN == 0)
        return;

    //! Сначала нам необходимо получить min и max эталонного графика
    double bMin = gBase->dataMainValue(0);
    double bMax = bMin;

    for (int i = 1; i < minN; ++i)
    {
        double const v = gBase->dataMainValue(i);
        bMin = std::min(bMin, v);
        bMax = std::max(bMax, v);
    }

    double dS = 0;
    double dS_min = 0;
    double dS_max = 0;

//    double const dX = (bMax - bMin);

    //! Пока считаем что временные отсчеты совпадают
    for (int i = 0; i < minN; ++i)
    {
        double const fx = gMeas->dataMainValue(i);
        double const x = gBase->dataMainValue(i);

//        double const fxx = gMeas->dataMainKey(i);
//        double const xx = gBase->dataMainKey(i);

        if (x == 0)
            continue;

        double const dd = fx - x;
        double const d = ((std::abs(dd) / x)) * 100;

        if (dd < 0)
            dS_min = std::min(dS_min, dd);
        else
            dS_max = std::max(dS_max, dd);

//        g1->addData(fxx, d);
//        g2->addData(fxx, dd * 3);

        dS += d;
    }

    QString result(QString::fromStdString(
            std::format("[ {:.1f}% ] [ > {:.1f} ] [ < {:.1f} ]",
                100 - (dS / minN), std::abs(dS_min), std::abs(dS_max))));
    lblErr_->setText(result);
}
