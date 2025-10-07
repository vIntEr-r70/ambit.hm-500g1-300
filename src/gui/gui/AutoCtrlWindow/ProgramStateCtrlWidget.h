#pragma once

#include <array>
#include <QFrame>

#include "common/Defs.h"

class OpItem;

class QLabel;
class Label;
class QPushButton;
class QHBoxLayout;
class QGridLayout;
class QBoxLayout;

class ProgramStateCtrlWidget
    : public QFrame
{
    Q_OBJECT

public:

    enum ProcCmd
    {
        CmdStart = 0,
        CmdStop,
        CmdContinue
    };

    enum
    {
        lblTotalTime,
        lblWaitTime,
        lblPower,
        lblCurrent,
        lblFrequency,
        lblCoolantT,
        lblPassage,

        lblCount
    };

public:

    ProgramStateCtrlWidget(QWidget*);

public:

    void resetStartButton();

private slots:

    void onCmdButton(int);

signals:

    void startCG();

    void stopCG();

    void continueCG();

private:

    void updateGui();

    void updateDtValue();

    QPushButton* createButton(QString const&);

    Label* createLabel(QBoxLayout*, QString const&, int = 80);

    void attachToMap(QLabel*, OpItem const*);

private:

    QPushButton* btnStart_;
    QPushButton* btnStop_;
    QPushButton* btnContinue_;

    std::array<Label*, lblCount> lbls_;

    QHBoxLayout* hL_;
    QGridLayout* gL_ = nullptr;

    uint8_t status_ = Core::ModeStatus::NotReady;
    uint8_t sysMode_ = Core::SysMode::No;

    struct DtInfo
    {
        float val;
        bool err;
    };
    std::array<DtInfo, 2> dt_;
    std::size_t dtMId_{ 0 };
};

