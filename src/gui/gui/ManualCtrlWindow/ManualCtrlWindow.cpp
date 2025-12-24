#include "ManualCtrlWindow.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmapCache>

#include "axis-ctl-widget/AxisCtlWidget.hpp"

#include "SprayerCtrlWidget.h"
#include "CenteringCfgWidget.h"

#include <FcCtrlWidget/FcCtrlWidget.h>

#include <axis-cfg.h>
#include <global.h>

#include <aem/log.h>

ManualCtrlWindow::ManualCtrlWindow(QWidget *parent, we::axis_cfg const &axis_cfg) noexcept
    : QWidget(parent) 
    , rpc_(global::rpc())
    , axis_cfg_(axis_cfg)
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
            spCtrlW_[0] = new SprayerCtrlWidget(this, "Sp-0",  "вода 1", true, true);
            spCtrlW_[1] = new SprayerCtrlWidget(this, "Sp-1",  "вода 2", true, true);
            spCtrlW_[2] = new SprayerCtrlWidget(this, "Sp-2",  "воздух", false, true);

            for (auto w : spCtrlW_)
                hL->addWidget(w);
        }
        vL->addLayout(hL);

        vL->addStretch();

        hL = new QHBoxLayout();
        hL->setSpacing(15);
        {
            fcCtrlW_ = new FcCtrlWidget(this, "fc", global::rpc(), global::signer());
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

    global::subscribe("sys.bki-allow", [this](nlohmann::json::array_t const&, nlohmann::json const& value)
    {
        bool bki_allow = value.get<bool>();
        cCfgW_->setEnabled(bki_allow);
    });
}

void ManualCtrlWindow::nf_sys_mode(unsigned char v) noexcept
{
    /* fcCtrlW_->nf_sys_mode(v); */
    cCfgW_->nf_sys_mode(v);

    for (auto w : spCtrlW_)
        w->nf_sys_mode(v);
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
    rpc_.call("set", { "cnc", "axis-calibrate", { axisId } })
        .error([this](std::string_view emsg)
        {
            aem::log::error("ManualCtrlWindow::onDoCalibrate: {}", emsg);
        });
}

void ManualCtrlWindow::engineMakeAsZero(char axisId)
{
    rpc_.call("set", { "cnc", "axis-init-zero", { axisId } })
        .error([this](std::string_view emsg)
        {
            aem::log::error("ManualCtrlWindow::onMakeZero: {}", emsg);
        });
}

void ManualCtrlWindow::engineApplyNewPos(char axisId, float pos)
{
    rpc_.call("set", { "cnc", "axis-move-to", { axisId, pos } });
}

