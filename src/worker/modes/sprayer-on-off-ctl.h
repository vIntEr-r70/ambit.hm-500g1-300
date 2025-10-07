#pragma once

#include "unit-on-off-ctl.h"

class sprayer_on_off_ctl final
    : public unit_on_off_ctl
{
public:

    sprayer_on_off_ctl(std::string_view, we::hardware &) noexcept;

private:

    bool is_locked() const noexcept override final;
};
