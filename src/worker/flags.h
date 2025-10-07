#pragma once

#include <cstddef>

namespace flags
{
    enum : std::size_t
    {
        bki_lock_ignore = 0,
        do_reset_bki,
        bki_not_allow,
    };
};
