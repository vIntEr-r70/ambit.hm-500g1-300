#pragma once

#include <eng/sibus/sibus.hpp>
#include <eng/json.hpp>
#include <eng/log.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <bitset>

namespace ambit
{

    enum class lock_status : std::uint8_t
    {
        normal,
        warning,
        critical
    };


    struct lock_system
    {
        struct subsystem_t
        {
            std::string name;
            eng::sibus::output_port_id_t port;
            std::vector<std::size_t> lockers;
            std::optional<bool> locked;
        };

        struct value_locker_t
        {
            std::string name;
            eng::sibus::output_port_id_t port;
            std::vector<std::size_t> subsystems;
            std::optional<lock_status> status;
            double value;
            std::array<double, 4> range;
        };

        struct flag_locker_t
        {
            std::string name;
            eng::sibus::output_port_id_t port;
            std::vector<std::size_t> subsystems;
            std::optional<lock_status> status;
            std::optional<bool> value;
            bool lock_by_value;
        };

        template <typename T>
        static bool load_config(std::filesystem::path const &fname, std::vector<T> &lockers, std::vector<subsystem_t> &subsystems)
        {
            try
            {
                eng::json::object cfg(fname);

                cfg.get_object("lockers").for_each([&](std::string_view sv_key, eng::json::value)
                {
                    lockers.emplace_back(std::string(sv_key));
                });

                cfg.get_object("subsystems").for_each([&](std::string_view sv_key, eng::json::value)
                {
                    subsystems.emplace_back(std::string(sv_key));
                });

                return true;
            }
            catch(std::exception const &e)
            {
                eng::log::error("ambit::lock_system::load_config[{}]: {}", fname.filename().native(), e.what());
            }

            return false;
        }

        template <typename T>
        static void configure_relationship(std::size_t idx, std::uint32_t mask, std::vector<T> &lockers, std::vector<subsystem_t> &subsystems)
        {
            auto &locker = lockers[idx];
            locker.subsystems.clear();

            // Маска контроллируемых данным параметром подсистем
            std::bitset<sizeof(mask) * CHAR_BIT> bits(mask);
            for (std::size_t i = 0; i < subsystems.size(); ++i)
            {
                auto &subsystem = subsystems[i];
                auto it = std::ranges::find(subsystem.lockers, idx);

                if (!bits.test(i))
                {
                    if (it != subsystem.lockers.end())
                        subsystem.lockers.erase(it);
                    continue;
                }
                locker.subsystems.push_back(i);

                if (it == subsystem.lockers.end())
                    subsystem.lockers.push_back(idx);
            }
        }

        template <typename T>
        static bool update_subsystem(std::size_t idx, std::vector<T> &lockers, std::vector<subsystem_t> &subsystems)
        {
            auto &ss = subsystems[idx];

            // Ищем хоть одно критическое состояние
            auto critical = std::ranges::find_if(ss.lockers,
                [&](std::size_t idx) {
                    return lockers[idx].status == ambit::lock_status::critical;
                });

            bool locked = (critical != ss.lockers.end());

            // Состояние не изменилось
            if (ss.locked && (*ss.locked == locked))
                return false;

            ss.locked = locked;

            return true;
        }
    };


}
