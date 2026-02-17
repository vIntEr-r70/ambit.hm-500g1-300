#pragma once

#include "../common/program-model.hpp"

#include <unordered_map>

class program_model_mode final
    : public program_model
{
    std::unordered_map<std::size_t, std::size_t> loop_repeated_;

public:

    program_model_mode() = default;

public:

    void set_phase_id(std::size_t);

    void reset_phase_id();

    void reset_loop();

private:

    std::size_t loop_repeat_count(std::size_t, std::size_t) const override final;
};

