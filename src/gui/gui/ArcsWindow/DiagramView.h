#pragma once

#include <QWidget>

#include <Widgets/qcustomplot.h>

#include "ArcInfo.h"
#include "ArcData.h"

class QLabel;

class DiagramView
    : public QWidget
{
public:

    DiagramView(ArcInfo::DgmDesc const&, ArcData const&, ArcData const&);

public:

    void makeUpdate();

private:

    void appendError() noexcept;

public:

    QCustomPlot plot_;

private:

    ArcData const& arcData_;
    ArcData const& arcBaseData_;

    QLabel* lblErr_;

    double min_{ std::numeric_limits<double>::max() };
    double max_{ std::numeric_limits<double>::min() };

    std::size_t cId_;
    std::size_t rows_{ 0 };
    std::size_t baseRows_{ 0 };
};


