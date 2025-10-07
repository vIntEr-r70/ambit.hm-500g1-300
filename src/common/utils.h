#pragma once

#include <cmath>

namespace utils
{
    template< class Enum >
    constexpr std::underlying_type_t<Enum> to_underlying( Enum e ) noexcept {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }

    namespace fc
    {
        static aem::uint16 to_reg_value(float v, double k)
        {
            return std::lround(v * k);
        }
    }
}
