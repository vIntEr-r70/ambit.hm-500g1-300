#pragma once

#include <string>
#include <bitset>
#include <vector>
#include <list>
#include <cstdint>
#include <ctime>

struct program_record_t
{
    static constexpr std::size_t hd_bit { 0 };
    static constexpr std::size_t usb_bit{ 1 };

    std::string filename;
    std::time_t last_write_date;

    std::uint32_t crc;
    std::string comments;

    std::bitset<2> source;
    bool removed;
    bool syncing;

    std::vector<std::byte> data;
    std::size_t head_size;

    std::list<program_record_t>::iterator iterator;
};

