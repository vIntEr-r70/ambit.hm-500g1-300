#pragma once

#include <state-ctrl.h>
#include <aem/countdown.h>

#include <drivers/bit-driver.h>

namespace we {
    class hardware;
}

class bki_allow_ctrl final
    : public state_ctrl<bki_allow_ctrl>
{
public:

    bki_allow_ctrl(std::string_view, we::hardware &) noexcept;

private:

    void on_activate() noexcept override final;

    void on_deactivate() noexcept override final;

private:

    void bki_allow() noexcept;

    void error() noexcept;

private:

    we::hardware &hw_;
    engine::bit_driver allow_;
};

