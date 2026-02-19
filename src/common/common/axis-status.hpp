#pragma once

#include <cstddef>

namespace ambit
{

    struct axis_status
    {
        static constexpr std::size_t ros{ 0 };
        static constexpr std::size_t fos{ 1 };
        static constexpr std::size_t fault{ 2 };
    };

}
