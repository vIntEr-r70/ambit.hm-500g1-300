#pragma once

#include <QWidget>

class QScrollArea;

class ArcDataViewWidget final
    : public QWidget
{
    Q_OBJECT

public:

    ArcDataViewWidget(QWidget*) noexcept;

signals:

    void go_back();

private:

    QScrollArea *scrollArea_;
};
