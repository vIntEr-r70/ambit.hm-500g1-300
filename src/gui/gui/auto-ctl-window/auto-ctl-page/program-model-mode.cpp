#include "program-model-mode.hpp"

void program_model_mode::set_phase_id(std::size_t id)
{
    // Проверяем, не выполняется ли цикл
    // if (current_row_ > id)
    // {
    //     // Запоминаем факт прохода цикла
    //     loop_repeated_[current_row_] += 1;
    //     dataChanged(createIndex(current_row_, 1), createIndex(current_row_ + 1, 1));
    // }
    // Мы завершили цикл, необходимо сбросить его счетчик
    // else if (id > current_row_ + 1)
    // {
    //     loop_repeated_[current_row_ + 1] = 0;
    //     dataChanged(createIndex(current_row_ + 1, 1), createIndex(current_row_ + 1, 1));
    // }

    if (current_row_ != program_.rows())
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, 0));

    current_row_ = id;

    if (current_row_ != program_.rows())
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, 0));

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
