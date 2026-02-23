#pragma once

#include "program.hpp"

#include <optional>

struct check_program
{
    // Цикл может осуществлять переход только на этап перед собой или выше
    static constexpr std::optional<std::size_t> find_bad_loop(program const &p)
    {
        std::size_t loop_idx = 0;
        for (std::size_t i = 0; i < p.phases.size(); ++i)
        {
            if (p.phases[i] != program::op_type::loop)
                continue;

            if (p.loop_ops[loop_idx].opid >= i)
                return i;

            loop_idx += 1;
        }
        return std::nullopt;
    }

    // Проверяем что во всей программе оси, способные вращаться используются только в одном режиме
    static constexpr std::optional<std::size_t> find_spin_vs_target_conflict(program const &p)
    {
        return std::nullopt;
    }

    // Проверяем что во всей программе оси, способные вращаться используются только в одном режиме
    static constexpr std::optional<std::size_t> find_zerro_speed(program const &p)
    {
        return std::nullopt;
    }

};
