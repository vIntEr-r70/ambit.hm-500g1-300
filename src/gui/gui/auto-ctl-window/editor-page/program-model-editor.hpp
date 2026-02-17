#pragma once

#include "../common/program-model.hpp"

class program_model_editor final
    : public program_model
{
public:

    program_model_editor() = default;

public:

    void set_current_row(std::size_t);

    void change_sprayer(std::size_t, std::size_t, std::size_t);

    void change_main(std::size_t, std::size_t, std::size_t, std::size_t, double);

    void change_pause(std::size_t, std::size_t, std::uint64_t);

    void change_loop(std::size_t, std::size_t, std::size_t, std::size_t);

    void change_fc(std::size_t, std::size_t, float, float, float);

    void change_center(std::size_t, std::size_t, centering_type, float);

    void add_op(program::op_type) noexcept;

    void add_main_op(bool) noexcept;

    void remove_op() noexcept;

    void reset() noexcept;

    bool empty() const noexcept;
};

