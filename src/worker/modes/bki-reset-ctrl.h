#pragma once

#include <state-ctrl.h>
#include <aem/countdown.h>

#include <drivers/bit-driver.h>

namespace we {
    class hardware;
}

class bki_reset_ctrl final
    : public state_ctrl<bki_reset_ctrl>
{
public:

    bki_reset_ctrl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void bki_allow() noexcept;

    void wait_allow_reset() noexcept;

    void set_reset_bit() noexcept;

    void wait_timeout() noexcept;

    void clear_reset_bit() noexcept;

    void error() noexcept;

private:

    we::hardware &hw_;

    engine::bit_driver reset_;

    aem::countdown countdown_;

    bool bki_lock_state_{ false };
    bool may_skip_bki_update_{ false };
};

