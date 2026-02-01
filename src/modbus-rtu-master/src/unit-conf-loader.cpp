#include "unit-conf-loader.hpp"

#include <eng/json.hpp>
#include <eng/modbus/regs-map.hpp>

#include <print>

namespace unit_conf_loader
{

    bool load(std::filesystem::path const &unit_config_file_name, callback cb)
    {
        try
        {
            eng::json::array(unit_config_file_name).for_each([&](eng::json::value v)
            {
                eng::json::object obj(v);

                std::string_view access = obj.get_string("access", "r-");
                std::size_t count = obj.get<std::uint8_t>("count", 1);

                if (count == 0)
                    throw std::runtime_error("invalid configuration values");

                std::array<std::uint8_t, 8> order{ 1, 0, 3, 2, 5, 4, 7, 6 };
                if (eng::json::array array(obj.get_array("order")); array)
                {
                    std::size_t idx = 0;
                    array.for_each([&order, &idx](eng::json::value value) {
                        order[idx++] = value.get<std::uint8_t>();
                    });
                }

                cb(record_t{
                        .address = obj.get<std::uint16_t>("address"),
                        .access = { access[0], access[1] },
                        .items_count = count,
                        .pool_time = obj.get<std::uint32_t>("pool-time", 1000),
                        .tag = obj.get_string("tag"),
                        .type = obj.get_string("type", "u16"),
                        .order = order
                    });
            });
        }
        catch(std::exception const &e)
        {
            std::println("ERROR: {}", e.what());
            return false;
        }

        return true;
    }

}

