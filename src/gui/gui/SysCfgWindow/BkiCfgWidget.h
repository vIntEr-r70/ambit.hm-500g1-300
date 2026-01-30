#pragma once

#include <QWidget>

class ValueSetBool;

class BkiCfgWidget
    : public QWidget
{
public:

    BkiCfgWidget(QWidget*);

private:

    void on_bki_status();

private:

    ValueSetBool *vsb_;
};
