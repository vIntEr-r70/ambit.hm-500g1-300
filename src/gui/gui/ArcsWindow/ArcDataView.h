#pragma once

#include <QWidget>

class QSlider;
class QScrollBar;
class QLineEdit;
class QVBoxLayout;
class ArcData;
class QLabel;

class DiagramView;

class ArcDataView
    : public QWidget
{
    Q_OBJECT

public:

    ArcDataView(ArcData&, ArcData&);

public:

    void newDataAppended();

private:

    int range() const noexcept;

private:

    ArcData& arcData_;
    ArcData& arcBaseData_;

    QScrollBar* sb_;
    QSlider* leftSlider_;
    QSlider* rightSlider_;

    QLabel* arcBeginDate_;
    QLabel* arcEndDate_;
    QLabel* arcDuration_;
    QLineEdit* arcUnitId_;
    QLabel* arcProgName_;

    std::vector<DiagramView*> dv_;
};


