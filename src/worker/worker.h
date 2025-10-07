#pragma once

#include <aem/timer.h>
#include <we/worker.h>
#include <state-ctrl.h>
#include <axis-cfg.h>
#include <drivers/cnc-driver.h>

#include "modes/auto/auto-mode.h"
#include "modes/manual/manual-mode.h"

#include "modes/bki-reset-ctrl.h"
#include "modes/bki-allow-ctrl.h"

class worker
    : public we::base_worker
    , public state_ctrl<worker>
{
public:

    worker(we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void update_cnc_configs() noexcept;

    void cfg_error() noexcept;

    void send_cnc_config_command() noexcept;

    void update_worker_mode() noexcept;

    void in_auto_mode() noexcept;

    void wait_auto_mode_done() noexcept;

    void in_manual_mode() noexcept;

    void wait_manual_mode_done() noexcept;
    
    void in_emg_stop_mode() noexcept;

    void init_lock_mode() noexcept;

    void wait_work_allow() noexcept;
        
private:

    void update_sys_mode(aem::uint8, bool = false) noexcept;

private:

    aem::timer timer_;
    we::axis_cfg axis_cfg_;

    cnc_driver cnc_;

    // Положение тумблера Ручной/Автоматический
    bool auto_mode_enabled_{ false };
    
    // Состояние аварийного выключателя 
    bool emg_stop_enabled_{ false };

    std::filesystem::path bki_path_;

    // Ручной/Автоматический режимы
    auto_mode auto_mode_;
    manual_mode manual_mode_;

    // Контроллер сброса БКИ
    bki_reset_ctrl bki_reset_ctrl_;
    bki_allow_ctrl bki_allow_ctrl_;

    aem::uint8 sys_mode_{ Core::SysMode::No };

    std::vector<std::string> cnc_config_queue_;

    bool conductor_0{ false };
    bool conductor_1{ false };

    bool login_{ false };
};
