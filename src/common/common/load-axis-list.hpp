#pragma once

#include <eng/log.hpp>
#include <eng/json.hpp>

#include <cstdlib>
#include <filesystem>

namespace ambit
{

    template <typename F>
    constexpr static bool load_axis_list(F f)
    {
        char const *LIAEM_RO_PATH = std::getenv("LIAEM_RO_PATH");
        if (LIAEM_RO_PATH == nullptr)
            return false;

        std::filesystem::path path(LIAEM_RO_PATH);
        path /= "axis.json";

        try
        {
            eng::json::array cfg(path);
            cfg.for_each([&f](std::size_t, eng::json::value v)
            {
                eng::json::object obj(v);

                char axis               = obj.get_sv("axis")[0];
                std::string_view name   = obj.get_sv("name", std::string(1, axis));
                bool rotation           = obj.get_bool("rotation", false);

                f(axis, name, rotation);
            });
        }
        catch(std::exception const &e)
        {
            eng::log::error("ambit::load-axis-list: {}", e.what());
            return false;
        }

        return true;
    }

}
