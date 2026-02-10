#include "ManualCtrlWindow.h"
#include "axis-ctl-widget/AxisCtlWidget.hpp"
#include "SprayerCtrlWidget.h"
#include "CenteringCfgWidget.h"

#include <FcCtrlWidget/FcCtrlWidget.h>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmapCache>

ManualCtrlWindow::ManualCtrlWindow(QWidget *parent) noexcept
    : QWidget(parent) 
{
    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    vL->setSpacing(10);
    {
        vL->addWidget(new AxisCtlWidget(this));

        vL->addStretch();

        QHBoxLayout* hL = new QHBoxLayout();
        hL->setSpacing(15);
        {
            hL->addWidget(new SprayerCtrlWidget(this, "sp0-gui-ctl",  "вода 1", true, true));
            hL->addWidget(new SprayerCtrlWidget(this, "sp1-gui-ctl",  "вода 2", true, true));
            hL->addWidget(new SprayerCtrlWidget(this, "sp2-gui-ctl",  "воздух", false, true));
        }
        vL->addLayout(hL);

        vL->addStretch();

        hL = new QHBoxLayout();
        hL->setSpacing(15);
        {
            fcCtrlW_ = new FcCtrlWidget(this, "fc");
            hL->addWidget(fcCtrlW_);

            cCfgW_ = new CenteringCfgWidget(this);
            hL->addWidget(cCfgW_);

            hL->setStretch(0, 3);
            hL->setStretch(1, 2);
        }
        vL->addLayout(hL);
    }

    // global::subscribe("cnc.{}.{}", [this](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    // {
    //     try
    //     {
    //         char axisId = keys[0].get<std::string_view>()[0];
    //         std::string_view kparam = keys[1].get<std::string_view>();
    //
    //         auto it = axis_.find(axisId);
    //         if (it == axis_.end())
    //             return;
    //
    //         cnc_map_.apply(it->second, kparam, value);
    //
    //         if (kparam == "pos")
    //             manualEngineSetDlg_->set_position(axisId, value.get<float>());
    //     }
    //     catch(...)
    //     {
    //     }
    // });

    // global::subscribe("sys.bki-allow", [this](nlohmann::json::array_t const&, nlohmann::json const& value)
    // {
    //     bool bki_allow = value.get<bool>();
    //     cCfgW_->setEnabled(bki_allow);
    // });
}

void ManualCtrlWindow::nf_sys_mode(unsigned char v) noexcept
{
    /* fcCtrlW_->nf_sys_mode(v); */
    cCfgW_->nf_sys_mode(v);
}

void ManualCtrlWindow::nf_sys_calibrate(char v) noexcept
{
    // manualEngineSetDlg_->set_calibrate_axis(v);
}

void ManualCtrlWindow::nf_sys_calibrate_step(int v) noexcept
{
    // manualEngineSetDlg_->set_calibrate_step(v);
}

void ManualCtrlWindow::nf_sys_centering_step(int v) noexcept
{
    cCfgW_->set_centering_step(v);
}

void ManualCtrlWindow::onDoCalibrate(char axisId) noexcept
{
    // rpc_.call("set", { "cnc", "axis-calibrate", { axisId } })
    //     .error([this](std::string_view emsg)
    //     {
    //         aem::log::error("ManualCtrlWindow::onDoCalibrate: {}", emsg);
    //     });
}

void ManualCtrlWindow::engineMakeAsZero(char axisId)
{
    // rpc_.call("set", { "cnc", "axis-init-zero", { axisId } })
    //     .error([this](std::string_view emsg)
    //     {
    //         aem::log::error("ManualCtrlWindow::onMakeZero: {}", emsg);
    //     });
}

void ManualCtrlWindow::engineApplyNewPos(char axisId, float pos)
{
    // rpc_.call("set", { "cnc", "axis-move-to", { axisId, pos } });
}

