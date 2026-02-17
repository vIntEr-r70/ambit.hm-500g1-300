#include "program-model-mode.hpp"

#include <eng/log.hpp>

void program_model_mode::set_phase_id(std::size_t id)
{
    if (current_row_ != program_.phases.size())
    {
        // Проверяем, не выполняется ли цикл
        if (id <= current_row_)
        {
            // Запоминаем факт прохода цикла
            std::size_t idx = current_row_;
            if (program_.phases[idx] != program::op_type::loop)
                idx += 1;
            loop_repeated_[idx] += 1;

            dataChanged(createIndex(current_row_, 1), createIndex(current_row_ + 1, 1));
        }
        // Мы завершили цикл, необходимо сбросить его счетчик
        else
        {
            if (loop_repeated_[id - 1] != 0)
            {
                loop_repeated_[id - 1] = 0;
                dataChanged(createIndex(id - 1, 1), createIndex(id - 1, 1));
            }
        }
    }

    if (current_row_ != program_.rows())
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, 0));

    current_row_ = id;

    if (current_row_ != program_.rows())
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, 0));

}

void program_model_mode::reset_loop()
{
    loop_repeated_.clear();
}

void program_model_mode::reset_phase_id()
{
    set_phase_id(program_.rows());
}

std::size_t program_model_mode::loop_repeat_count(std::size_t id, std::size_t N) const
{
    auto it = loop_repeated_.find(id);
    return (it == loop_repeated_.end()) ? N : N - it->second;
}
