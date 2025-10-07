#include "worker.h"
#include "flags.h"

#include <hardware.h>
#include <aem/time_span.h>

worker::worker(we::hardware &hw) noexcept
    : we::base_worker(hw)
    , state_ctrl("worker")
    , axis_cfg_(aem::getenv("AMBIT_HOME_PATH", "/home/alex/projects/ambit/RmCtrl/cfg"))
    , cnc_(hw.get_unit_by_class("", "cnc"))
    , auto_mode_(hw, axis_cfg_)
    , manual_mode_(hw, axis_cfg_)
    , bki_reset_ctrl_("bki-reset-ctrl", hw)
    , bki_allow_ctrl_("bki-allow-ctrl", hw)
{
    bki_path_ = aem::getenv("AMBIT_CFG_PATH", "/home/alex/projects/ambit/RmCtrl/cfg");
    bki_path_ /= "bki-not-allow";
    hw_.set_flag(flags::bki_not_allow, std::filesystem::exists(bki_path_));

    aem::setDeferredCall(aem::time_span::zero(), [this] {
        hw_.notify({ "sys", "bki-allow" }, !hw_.get_flag(flags::bki_not_allow));
    });

    hw.GET.add("cnc", "axis-cfg", [this](nlohmann::json const &json)
    {
        nlohmann::json result;

        if (!json.empty())
        {
            char axis = json[0].get<char>();
            result[std::string(1, axis)] = axis_cfg_.base64(axis);
        }
        else
        {
            for (auto const& item : axis_cfg_)
                result[std::string(1, item.first)] = axis_cfg_.base64(item.first);
        }

        return result;
    });

    hw.SET.add("cnc", "axis-cfg", [this](nlohmann::json const &args)
    {
        char axis = args[0].get<char>();
        if (!axis_cfg_.base64(axis, args[1].get<std::string_view>()))
            return;

        manual_mode_.panel_mode_.panel_ctrl_.axis_cfg_changed();
        manual_mode_.panel_mode_.move_to_ctrl_.axis_cfg_changed();
        manual_mode_.rcu_ctrl_.axis_cfg_changed();

        axis_cfg_.save(axis);
    });

    hw.SET.add("sys", "login", [this](nlohmann::json const &args)
    {
        login_ = args[0].get<bool>();
    });

    hw.SET.add("bki", "allow", [this](nlohmann::json const &args)
    {
        if (args.get<bool>())
            std::filesystem::remove(bki_path_);
        else
            std::ofstream output(bki_path_);
    });

    hw.LSET.add(name(), "mode-auto", [this](nlohmann::json const &value) {
        auto_mode_enabled_ = value.get<bool>();
    });

    hw.LSET.add(name(), "emg-stop", [this](nlohmann::json const &value)
    {
        bool emg_stop_enabled = value.get<bool>();
        if (emg_stop_enabled && !emg_stop_enabled_)
            hw_.log_message(LogMsg::Error, name(), "Задействован аварийный СТОП");
        emg_stop_enabled_ = emg_stop_enabled;
    });
}

void worker::on_activate() noexcept
{
    update_sys_mode(Core::SysMode::No, true);

    we::base_worker::ctl_activate();
    bki_reset_ctrl_.activate();
    bki_allow_ctrl_.activate();

    // Формируем массив конфигураций CNC
    next_state(&worker::update_cnc_configs);
}

void worker::on_deactivate() noexcept
{
    we::base_worker::ctl_deactivate();
}

/////////////////////////////////////////////////////////////////////

void worker::update_cnc_configs() noexcept
{
    // Коэфициент пропорциональности
    float min_acc = NAN;
    for (auto const& axis : axis_cfg_)
    {
        if (!axis.second.use())
            continue;

        if (std::isnan(min_acc))
            min_acc = axis.second.acc;
        else
            min_acc = std::min(min_acc, axis.second.acc);

        std::size_t id = we::axis_cfg::ID(axis.first);

        cnc_config_queue_.push_back(fmt::format("${}={:.3f}\r", 20 + id, axis.second.ratio));
    }

    if (!std::isnan(min_acc))
        cnc_config_queue_.push_back(fmt::format("$8={:.3f}\r", min_acc));

    // Маска инверсии концевиков
    aem::uint16 mask = 0;
    for (auto const& axis : axis_cfg_)
    {
        if (!axis.second.use())
            continue;

        //! Инверсия задается только первым 6 осям (X,Y,Z,A,B,C)
        std::size_t id = we::axis_cfg::ID(axis.first);
        if (id > 5) continue;

        aem::uint16 place = axis.second.limitSI;
        mask |= (place << (id * 2));
    }
    cnc_config_queue_.push_back(fmt::format("$14={}\r", mask));

    // Выставляем все в независимое движение
    cnc_config_queue_.push_back("M200 P1023\r");
    cnc_.reset_independent_mask();

    next_state(&worker::send_cnc_config_command);
}

void worker::send_cnc_config_command() noexcept
{
    auto [ done, error ] = cnc_.string_command(cnc_config_queue_.back());
    if (!dhresult::check(done, error, [this] { next_state(&worker::cfg_error); }))
        return;

    cnc_config_queue_.pop_back();

    if (cnc_config_queue_.empty())
        next_state(&worker::update_worker_mode);
    else
        next_state(&worker::send_cnc_config_command);
}

void worker::cfg_error() noexcept
{
}

void worker::update_worker_mode() noexcept
{
    // if (!login_ || hw_.is_locked(we::locker::lock_bits::moving))
    // {
    //     next_state(&worker::init_lock_mode);
    //     return;
    // }

    if (emg_stop_enabled_)
    {
        hw_.send_event("worker", "block-manual", true);

        update_sys_mode(Core::SysMode::No);
        next_state(&worker::in_emg_stop_mode);
        return;
    }

    hw_.send_event("worker", "block-manual", auto_mode_enabled_);

    if (auto_mode_enabled_)
    {
        update_sys_mode(Core::SysMode::Auto);

        auto_mode_.activate();
        next_state(&worker::in_auto_mode);
    }
    else
    {
        update_sys_mode(Core::SysMode::Manual);

        manual_mode_.activate();
        next_state(&worker::in_manual_mode);
    }
}

void worker::in_auto_mode() noexcept
{
    // if (auto_mode_enabled_ && !emg_stop_enabled_ && !hw_.is_locked(we::locker::lock_bits::moving) && login_)
    //     return;

    auto_mode_.deactivate();
    next_state(&worker::wait_auto_mode_done);
}

void worker::wait_auto_mode_done() noexcept
{
    if (!auto_mode_.is_done())
        return;
    next_state(&worker::update_worker_mode);
}

void worker::in_manual_mode() noexcept
{
    // if (!auto_mode_enabled_ && !emg_stop_enabled_ && !hw_.is_locked(we::locker::lock_bits::moving) && login_)
    //     return;
    manual_mode_.deactivate();

    next_state(&worker::wait_manual_mode_done);
}

void worker::wait_manual_mode_done() noexcept
{
    if (!manual_mode_.is_done())
        return;
    next_state(&worker::update_worker_mode);
}

void worker::in_emg_stop_mode() noexcept
{
    if (emg_stop_enabled_)
        return;
    next_state(&worker::update_worker_mode);
}

void worker::init_lock_mode() noexcept
{
    next_state(&worker::wait_work_allow);
}

void worker::wait_work_allow() noexcept
{
    // if (hw_.is_locked(we::locker::lock_bits::moving) || !login_)
    //     return;
    next_state(&worker::update_worker_mode);
}

/////////////////////////////////////////////////////////////////////

void worker::update_sys_mode(aem::uint8 sys_mode, bool force) noexcept
{
    if (sys_mode_ == sys_mode && !force)
        return;
    sys_mode_ = sys_mode;
    hw_.nf_.notify({ "sys", "mode" }, sys_mode_);
}

