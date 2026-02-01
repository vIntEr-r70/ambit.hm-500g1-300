#include "unit-conf-loader.hpp"

#include <eng/json.hpp>
#include <eng/modbus/regs-map.hpp>
#include <eng/log.hpp>

namespace unit_conf_loader
{

    bool load(std::filesystem::path const &unit_config_file_name, callback cb)
    {
        try
        {
            eng::json::object conf(unit_config_file_name);

            auto host = conf.get_string("host");
            auto port = conf.get_uint("port", 502);

            conf.get_array("regs").for_each([&](std::size_t, eng::json::value v)
            {
                eng::json::object obj(v);

                std::string_view access = obj.get_string("access", "r-");
                std::size_t count = obj.get<std::uint8_t>("count", 1);

                if (count == 0)
                    throw std::runtime_error("invalid configuration values");

                std::array<std::uint8_t, 8> order{ 1, 0, 3, 2, 5, 4, 7, 6 };
                if (eng::json::array array(obj.get_array("order")); array)
                {
                    array.for_each([&order](std::size_t idx, eng::json::value value) {
                        order[idx] = value.get<std::uint8_t>();
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
            eng::log::error("{}", e.what());
            return false;
        }

        return true;
    }

    bool load(std::filesystem::path const &unit_config_file_name, unit_conf_t &conf)
    {
        try
        {
            eng::json::object obj(unit_config_file_name);

            conf.host = obj.get_string("host");
            conf.port = obj.get_uint("port", 502);

            obj.get_array("regs").for_each([&](std::size_t, eng::json::value v)
            {
                eng::json::object obj(v);

                conf.records.emplace_back();
                auto &rec = conf.records.back();

                std::string_view access = obj.get_string("access", "r-");
                std::size_t count = obj.get<std::uint8_t>("count", 1);

                if (count == 0)
                    throw std::runtime_error("invalid configuration values");

                rec.address = obj.get<std::uint16_t>("address");
                rec.access = { access[0], access[1] };
                rec.items_count = count;
                rec.pool_time = obj.get<std::uint32_t>("pool-time", 1000);
                rec.tag = obj.get_string("tag");
                rec.type = obj.get_string("type", "u16");

                rec.order = { 1, 0, 3, 2, 5, 4, 7, 6 };
                if (eng::json::array array(obj.get_array("order")); array)
                {
                    array.for_each([&rec](std::size_t idx, eng::json::value value) {
                        rec.order[idx] = value.get<std::uint8_t>();
                    });
                }
            });
        }
        catch(std::exception const &e)
        {
            eng::log::error("{}", e.what());
            return false;
        }

        return true;
    }

}

