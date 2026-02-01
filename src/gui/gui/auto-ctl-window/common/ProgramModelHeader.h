#pragma once

#include <cstddef>
#include <utility>

#include <QString>

class program;
class QTableWidget;

class ProgramModelHeader
{
public:

    enum : std::size_t
    {
        Num,
        FcI,
        FcP,
        Sprayer,
        Spin,
        TargetPos,
        TargetSpeed
    };

public:

    static std::pair<std::size_t, std::size_t> cell(program const &, std::size_t) noexcept; 

    static std::size_t column_count(program const&) noexcept;

    static void create_header(program const&, QTableWidget&, int) noexcept;

    static QString title(program const&, std::size_t, std::size_t) noexcept;
};
