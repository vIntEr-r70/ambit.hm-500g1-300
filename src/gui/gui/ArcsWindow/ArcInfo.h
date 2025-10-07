#pragma once

#include <vector>
#include <map>
#include <string>
#include <cstdint>

// #include "Libs/rapidjson/document.h"

// #include "Ex/Buffer.h"

class ArcInfo
{
    // typedef void (ArcInfo::*Handler)(rapidjson::Value const&);

public:

    typedef uint32_t HeadSizeType;

public:

    struct DgmDesc
    {
        std::string title;
        std::string id;
        std::size_t cId;
        bool asFlags{ true };
    };

public:

    void reset() noexcept;

    bool append(uint8_t const*, std::size_t, std::size_t* = nullptr) noexcept;

    inline std::size_t headerSize() const noexcept { return headerSize_; }

    inline bool inited() const noexcept { return (headerSize_ != 0); }

private:

//    Handler handler(char const*) const;

    // void append(rapidjson::Value const&, rapidjson::Value const&);

private:

    // void asInfo(rapidjson::Value const&);

    // void asHeader(rapidjson::Value const&);

public:

    std::map<std::string, std::string> info;

    std::vector<char const*> cols;
    std::vector<DgmDesc> dgms;

private:

    std::size_t headerSize_{ 0 };

    // rapidjson::MemoryPoolAllocator<> allocator_;
    // rapidjson::Document doc_;    

    // Ex::Buffer data_;
};

