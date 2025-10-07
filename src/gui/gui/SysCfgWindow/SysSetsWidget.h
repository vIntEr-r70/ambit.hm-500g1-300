#pragma once

#include <QWidget>

#include "func-map.h"

class QLabel;
class ValueSetString;
class ValueSetInt;

class SysSetsWidget
    : public QWidget
{
    enum : std::size_t
    {
        lIdMainTitle0,
        lIdMainTitle1,
        lIdUnitId
    };

    enum
    {
        lblDay,
        lblMonth,
        lblYear,
        lblHour,
        lblMin,

        lblCounts
    };

public:

    SysSetsWidget(QWidget*) noexcept;

public:

    void set_guid(int);

public:

    void nf_sys_optime_cn(std::size_t);

    void nf_sys_optime_fc(std::size_t);

private:

    void on_value_changed();

    void initNotify();

    void showEvent(QShowEvent*);

    void makeApplyMainTitle();

    void makeApplyUnitId();

private:

    func_multi_map sys_key_map_;

    QLabel *cn_optime_;
    QLabel *fc_optime_;

    ValueSetString *vss_unit_name_;
    ValueSetString *vss_unit_sid_;

    ValueSetInt *vsi_hour_;
    ValueSetInt *vsi_mins_;

    ValueSetInt *vsi_day_;
    ValueSetInt *vsi_month_;
    ValueSetInt *vsi_year_;
};

