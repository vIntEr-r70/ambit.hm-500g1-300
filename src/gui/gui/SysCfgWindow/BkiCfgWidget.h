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

    void load_config();

    void save_config();

private:

    ValueSetBool *vsb_;
};
