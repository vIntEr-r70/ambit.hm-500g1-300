#pragma once

#include <filesystem>
#include <functional>

namespace unit_conf_loader
{

    using order_t = std::array<std::uint8_t, 8>;

    struct record_t
    {
        std::uint16_t address;
        std::array<char, 2> access;
        std::size_t regs_per_item;
        std::size_t items_count;
        std::uint32_t pool_time;
        std::string_view tag;
        std::string_view type;
        order_t order;
    };

    using callback = std::function<void(record_t)>;

    bool load(std::filesystem::path const &, callback);

}

