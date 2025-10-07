#include "ProgramStateCtrlWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>

#include <Widgets/Label.h>

#include "common/ProgramOpItems.h"

ProgramStateCtrlWidget::ProgramStateCtrlWidget(QWidget* parent)
    : QFrame(parent)
{
    QFrame::setFrameStyle(QFrame::StyledPanel);

    QHBoxLayout* layout = new QHBoxLayout(this);

    QButtonGroup* group = new QButtonGroup(this);
    group->setExclusive(false);
    connect(group, SIGNAL(buttonClicked(int)), this, SLOT(onCmdButton(int)));

    btnStart_ = createButton("СТАРТ");
    group->addButton(btnStart_, CmdStart);
    layout->addWidget(btnStart_);
    btnStop_ = createButton("СТОП");
    group->addButton(btnStop_, CmdStop);
    layout->addWidget(btnStop_);
    btnContinue_ = createButton("ДАЛЕЕ");
    group->addButton(btnContinue_, CmdContinue);
    btnContinue_->setCheckable(false);
    layout->addWidget(btnContinue_);

    layout->addStretch();

    lbls_[lblTotalTime] = createLabel(layout, "Общее\nвремя");
    lbls_[lblTotalTime]->setStyleSheet("background-color: white");
    layout->addSpacing(5);
    lbls_[lblWaitTime] = createLabel(layout, "Время\nожидания");
    lbls_[lblWaitTime]->setStyleSheet("background-color: white");
    layout->addSpacing(5);
    lbls_[lblPower] = createLabel(layout, "Мощность\nкВт", 65);
    layout->addSpacing(5);
    lbls_[lblCurrent] = createLabel(layout, "Ток\nА", 65);
    lbls_[lblCurrent]->setStyleSheet("background-color: white");
    layout->addSpacing(5);
    lbls_[lblFrequency] = createLabel(layout, "Частота\nкГц", 65);
    lbls_[lblFrequency]->setStyleSheet("background-color: white");
    layout->addSpacing(5);
    lbls_[lblCoolantT] = createLabel(layout, "Вода\n°C", 65);
    layout->addSpacing(5);
    lbls_[lblPassage] = createLabel(layout, "Проток\nлитры/мин", 70);
    lbls_[lblPassage]->setStyleSheet("background-color: white");

    layout->addSpacing(10);

    // initNotify(SysCfg::inst().coreMap["state"]["auto-mode"]);
}

void ProgramStateCtrlWidget::attachToMap(QLabel* lbl, OpItem const* op)
{
    // OpPos const* pos = dynamic_cast<OpPos const*>(op);
    // if (pos != nullptr)
    // {
    //     Ex::Acs::Value value =
    //         SysCfg::inst().coreMap["state"]["cnc"][pos->id].getValue("pos");
    //
    //     value.connect([lbl](Ex::Acs::value const& v) {
    //         lbl->setText(QString::number(v.asFloat(), 'f' , 1));
    //     });
    // }
}

// void ProgramStateCtrlWidget::initNotify(Ex::Acs::Map const& map)
// {
//     auto const& rMap = SysCfg::inst().coreMap["state"];
//
//     map.getValue("status").connect([this](Ex::Acs::value const& v)
//     {
//         status_ = v.asUInt8();
//
// //        static QStringList const ttls(QStringList()
// //                << "Не готов" << "Ошибка" << "Готов" << "Выполняется" << "Ожидание");
// //        lbls_[0]->setText(ttls[status_]);
//
//         updateGui();
//     });
//
//     map.getValue("proc-time").connect([this](Ex::Acs::value const& v)
//     {
//         uint32_t tv = v.asUInt32();
//         QString tvText;
//         tvText.sprintf("%02u:%02u:%02u", tv / 3600, (tv % 3600) / 60, tv % 60);
//         lbls_[lblTotalTime]->setText(tvText);
//     });
//
//     map.getValue("wait-time").connect([this](Ex::Acs::value const& v)
//     {
//         uint32_t tv = v.asUInt32();
//
//         QString tvText;
//         if (tv == std::numeric_limits<uint32_t>::max())
//             tvText = "∞";
//         else
//             tvText.sprintf("%02u:%02u:%02u", tv / 3600, (tv % 3600) / 60, tv % 60);
//
//         lbls_[lblWaitTime]->setText(tvText);
//     });
//
//     rMap["sys"].getValue("mode").connect([this](Ex::Acs::value const& v)
//     {
//         sysMode_ = v.asUInt8();
//         updateGui();
//     });
//
//     rMap["fc"].getValue("f").connect([this](Ex::Acs::value const& v)
//     {
//         lbls_[lblFrequency]->setText(QString::number(v.asFloat() / 1000.0f, 'f', 1));
//     });
//
//     rMap["fc"].getValue("p").connect([this](Ex::Acs::value const& v)
//     {
//         lbls_[lblPower]->setText(QString::number(v.asFloat() / 1000.0f, 'f', 1));
//     });
//
//     rMap["fc"].getValue("i").connect([this](Ex::Acs::value const& v)
//     {
//         lbls_[lblCurrent]->setText(QString::number(v.asFloat(), 'f', 1));
//     });
//
//     rMap.getValue("sprayer").connect([this](Ex::Acs::value const& v)
//     {
//         lbls_[lblCoolantT]->setStyleSheet(v.asBool() ?
//             "background-color: rgb(0, 212, 0)" : "background-color: white");
//     });
//
//     rMap["fc"].getValue("turn-on").connect([this](Ex::Acs::value const& v)
//     {
//         lbls_[lblPower]->setStyleSheet(v.asBool() ?
//             "background-color: rgb(0, 212, 0)" : "background-color: white");
//     });
//
//     rMap["tx"].getValue("flow").connect([this](Ex::Acs::value const& v)
//     {
//         lbls_[lblPassage]->setText(QString::number(v.asUInt32()));
//     });
//
//     rMap["trm"].getArray("dt").connect([this](std::size_t iId, Ex::Acs::value const& v)
//     {
//         dt_[iId].val = v.asFloat();
//         updateDtValue();
//     });
//     rMap["trm"].getArray("err").connect([this](std::size_t iId, Ex::Acs::value const& v)
//     {
//         dt_[iId].err = v.asBool();
//         updateDtValue();
//     });
//
//     SysCfg::inst().coreMap["cfg"]["trm"].getValue("mid").connect([this](Ex::Acs::value const& v)
//     {
//         dtMId_ = std::min(std::size_t(v.asUInt8()), std::size_t(dt_.size() - 1));
//         updateGui();
//         updateDtValue();
//     });
// }

void ProgramStateCtrlWidget::updateGui()
{
    using St = Core::ModeStatus;

    bool allow = (sysMode_ == Core::SysMode::Auto);

    for (auto lbl : lbls_)
        lbl->setEnabled(allow);

    if (!allow)
    {
        btnStart_->setEnabled(false);
        btnStart_->setChecked(false);
        btnStart_->setStyleSheet("");

        btnStop_->setEnabled(false);
        btnStop_->setChecked(false);
        btnStop_->setStyleSheet("");

        btnContinue_->setVisible(true);
        btnContinue_->setEnabled(false);

        return;
    }

    bool inProc = (status_ == St::InProc) || (status_ == St::Wait);
    btnStart_->setChecked(inProc);
    btnStart_->setEnabled(!inProc && (status_ == St::Ready));
    btnStart_->setStyleSheet((status_ == St::InProc) ? "background-color: rgb(0, 212, 0)" : "");

    btnStop_->setChecked(!inProc);
    btnStop_->setEnabled(inProc);
    btnStop_->setStyleSheet(!inProc ? "background-color: rgb(237, 212, 0)" : "");

    btnContinue_->setEnabled(status_ == St::Wait);
    btnContinue_->setVisible(status_ == St::Wait);

    auto const& info = dt_[dtMId_];
    lbls_[lblCoolantT]->setEnabled(!info.err);
}

void ProgramStateCtrlWidget::updateDtValue()
{
    auto const& info = dt_[dtMId_];
    lbls_[lblCoolantT]->setText(QString::number(info.val, 'f', 1));
}

QPushButton* ProgramStateCtrlWidget::createButton(QString const& title)
{
    QPushButton* btn = new QPushButton(title, this);
    btn->setObjectName("btn");
    btn->setCheckable(true);
    btn->setMinimumHeight(55);
    btn->setMinimumWidth(100);
    return btn;
}

Label* ProgramStateCtrlWidget::createLabel(QBoxLayout* layout, QString const& ttl, int w)
{
    QVBoxLayout* vL = new QVBoxLayout();
    vL->setSpacing(3);

    Label* lbl = new Label(ttl, this);
    lbl->setAlignment(Qt::AlignCenter);
    vL->addWidget(lbl);

    lbl = new Label(this);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setFrameStyle(QFrame::Box);
    lbl->setMinimumHeight(25);
    lbl->setMinimumWidth(w);
    lbl->setDisableText("- - -");
    lbl->setDisableStyleSheet("");
    vL->addWidget(lbl);

    layout->addLayout(vL);

    return lbl;
}

void ProgramStateCtrlWidget::onCmdButton(int cmd)
{
    switch (cmd)
    {
    case CmdStart: emit startCG(); break;
    case CmdStop: emit stopCG(); break;
    case CmdContinue: emit continueCG(); break;
    default: break;
    }
}

void ProgramStateCtrlWidget::resetStartButton()
{
    btnStart_->setChecked(false);
}
