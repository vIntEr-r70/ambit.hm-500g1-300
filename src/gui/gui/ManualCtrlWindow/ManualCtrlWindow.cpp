#include "ManualCtrlWindow.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmapCache>

#include "AxisCtrlWidget.h"
#include <FcCtrlWidget/FcCtrlWidget.h>
#include "SprayerCtrlWidget.h"
#include "CenteringCfgWidget.h"

#include "ManualEngineSetDlg.h"

#include <Interact.h>

#include <axis-cfg.h>
#include <global.h>

#include <aem/log.h>

ManualCtrlWindow::ManualCtrlWindow(QWidget *parent, we::axis_cfg const &axis_cfg) noexcept
    : QWidget(parent) 
    , rpc_(global::rpc())
    , axis_cfg_(axis_cfg)
{
    manualEngineSetDlg_ = new ManualEngineSetDlg(this, axis_cfg_);
    manualEngineSetDlg_->setVisible(false);

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(10, 10, 10, 10);
    vL->setSpacing(10);
    {
        // Резервируем место под виджеты состояния и управления осями
        vL_ = new QVBoxLayout();
        vL_->setSpacing(15);
        vL->addLayout(vL_);

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

    // Создаем виджеты для всех осей
    std::for_each(axis_cfg_.begin(), axis_cfg_.end(), [this](auto &&it)
    {
        AxisCtrlWidget* w = new AxisCtrlWidget(this, it.first, it.second);
        connect(w, SIGNAL(onMoveToClick(char)), this, SLOT(onAxisWidgetMoveTo(char)));

        w->setMinimumWidth(240);
        w->setMaximumWidth(240);
        w->setMaximumHeight(110);
        w->setVisible(false);

        axis_[it.first] = w;
    });

    connect(manualEngineSetDlg_, SIGNAL(applyNewPos(char, float)), this, SLOT(engineApplyNewPos(char, float)));
    connect(manualEngineSetDlg_, SIGNAL(doCalibrate(char)), this, SLOT(onDoCalibrate(char)));
    connect(manualEngineSetDlg_, SIGNAL(doZero(char)), this, SLOT(engineMakeAsZero(char)));

    global::subscribe("cnc.{}.{}", [this](nlohmann::json::array_t const& keys, nlohmann::json const& value)
    {
        try
        {
            char axisId = keys[0].get<std::string_view>()[0];
            std::string_view kparam = keys[1].get<std::string_view>();

            auto it = axis_.find(axisId);
            if (it == axis_.end())
                return;

            cnc_map_.apply(it->second, kparam, value);

            if (kparam == "pos")
                manualEngineSetDlg_->set_position(axisId, value.get<float>());
        }
        catch(...)
        {
        }
    });

    global::subscribe("sys.bki-allow", [this](nlohmann::json::array_t const&, nlohmann::json const& value)
    {
        bool bki_allow = value.get<bool>();
        cCfgW_->setEnabled(bki_allow);
    });

    cnc_map_.add("speed", &AxisCtrlWidget::nf_speed);
    cnc_map_.add("lsmin", &AxisCtrlWidget::nf_ls_min);
    cnc_map_.add("lsmax", &AxisCtrlWidget::nf_ls_max);
    cnc_map_.add("pos", &AxisCtrlWidget::nf_pos);
}

void ManualCtrlWindow::nf_sys_error(unsigned int v) noexcept
{
    for (auto& item : axis_)
        item.second->set_sys_error(v);
}

void ManualCtrlWindow::nf_sys_mode(unsigned char v) noexcept
{
    /* fcCtrlW_->nf_sys_mode(v); */
    cCfgW_->nf_sys_mode(v);

    for (auto w : spCtrlW_)
        w->nf_sys_mode(v);

    for (auto& item : axis_)
        item.second->set_sys_mode(v);
}

void ManualCtrlWindow::nf_sys_ctrl(unsigned char v) noexcept
{
    for (auto& item : axis_)
        item.second->set_sys_ctrl(v);
}

void ManualCtrlWindow::nf_sys_ctrl_mode_axis(char v) noexcept
{
    for (auto& item : axis_)
        item.second->set_sys_ctrl_mode_axis(v);
}

void ManualCtrlWindow::nf_sys_calibrate(char v) noexcept
{
    manualEngineSetDlg_->set_calibrate_axis(v);
}

void ManualCtrlWindow::nf_sys_calibrate_step(int v) noexcept
{
    manualEngineSetDlg_->set_calibrate_step(v);
}

void ManualCtrlWindow::nf_sys_centering_step(int v) noexcept
{
    cCfgW_->set_centering_step(v);
}

void ManualCtrlWindow::apply_axis_cfg() noexcept
{
    std::for_each(axis_cfg_.begin(), axis_cfg_.end(), [this](auto &&it)
    {
        if (!it.second.use())
            return;
        addAxis(it.first);
    });
}

void ManualCtrlWindow::addAxis(char axisId) noexcept
{
    QHBoxLayout* hL = hL_.empty() ? nullptr : hL_.back();

    if (hL == nullptr || hL->count() == 4)
    {
        hL_.push_back(new QHBoxLayout());
        vL_->addLayout(hL_.back());
        hL = hL_.back();
        hL->setSpacing(15);
    }

    AxisCtrlWidget *w = axis_[axisId];
    w->apply_self_axis();
    w->setVisible(true);

    hL->addWidget(w);
}

void ManualCtrlWindow::onAxisWidgetMoveTo(char axisId) noexcept
{
    manualEngineSetDlg_->prepare(axisId);

    rpc_.call("set", { "cnc", "axis-stop-all", {} })
        .done([this](nlohmann::json const&)
        {
            // Показываем диалог
            Interact::dialog(manualEngineSetDlg_);
        })
        .error([this](std::string_view emsg)
        {
            aem::log::error("ManualCtrlWindow::onAxisWidgetClick: axis-stop-all: {}", emsg);
        });
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
