#include "vm.hpp"

#include <eng/log.hpp>

#include <cmath>
#include <algorithm>
#include <sstream>
#include <stdexcept>

vm::vm(std::vector<VmPhase*> const &phases, std::vector<char> const &target_axis) noexcept
    : phases_(phases)
    , target_axis_(target_axis)
{
    map_pos_operations_[VmOpType::Pos]      = &vm::_vm_next_op_pos;
    map_pos_operations_[VmOpType::Speed]    = &vm::_vm_next_op_speed;
}

void vm::initialize()
{
    phase_id_ = 0;

    ops_phases_.clear();
    ops_phases_.emplace_back(0);

    phase_moving_.clear();

    known_goto_.clear();

    // Начальная позиция равна 0.0
    std::ranges::for_each(target_axis_, [this](char axis)
    {
        phase_moving_[axis].position = 0.0;
    });
}

bool vm::is_program_done() const
{
    return phase_id_ == phases_.size();
}

VmPhaseType vm::phase_type() const
{
    if (phase_id_ >= phases_.size())
        throw std::runtime_error("phase_type: try to continue with finished programm");
    return phases_[phase_id_]->cmd();
}

std::size_t vm::next_phase_id(std::size_t pid) const
{
    if (pid >= phases_.size())
        return pid;

    // В случае когда этап не является циклом, возвращаем текущее значение
    // с которого будет произведен анализ и поиск операций
    VmPhase const *phase = phases_[pid];
    if (phase->cmd() != VmPhaseType::GoTo)
        return pid;

    // А если это цикл, мы должны понять, он еще актуален
    // или его можно проскочить
    VmGoTo const &item = *static_cast<VmGoTo const*>(phase);

    // Точка перехода может сама оказаться циклом, учитываем это

    auto it = known_goto_.find(pid);
    if ((it == known_goto_.end()) || (it->second > 0))
        return next_phase_id(item.phase_id);

    return next_phase_id(pid + 1);
}

std::size_t vm::next_phase_id() const
{
    return next_phase_id(phase_id_);
}

std::size_t vm::op_phase_id() const
{
    if (ops_phases_.empty())
        throw std::runtime_error("try to get wrong operation phase id");
    return ops_phases_.back();
}

std::size_t vm::to_next_phase()
{
    if (ops_phases_.empty())
        throw std::runtime_error("ops phase list was empty");

    phase_id_ = ops_phases_.back();
    ops_phases_.pop_back();

    return phase_id_;
}

void vm::increment_phase()
{
    if (!ops_phases_.empty())
        throw std::runtime_error("ops phase list was not empty");
    phase_id_ += 1;
}

std::uint64_t vm::pause_timeout_ms() const
{
    VmPhase const *phase = phases_[phase_id_];
    if (phase->cmd() != VmPhaseType::Pause)
        throw std::runtime_error("try get pause timeout from non pause phase");
    VmPause const &item = *static_cast<VmPause const*>(phase);
    return item.msec;
}

// Инициируем процесс формирования списка позиций непрерывного движения
bool vm::create_continuous_moving_list()
{
    eng::log::info("{}: phase-id = {}", __func__, phase_id_);

    if (is_program_done())
        return false;

    if (!ops_phases_.empty())
        throw std::runtime_error("create_continuous_moving_list: ops phase list was not empty");

    continuous_moving_list_.clear();

    std::ranges::for_each(phase_moving_, [](auto &pair)
    {
        auto &axis = pair.second;
        axis.moving_direction = 0;
    });

    std::size_t phase_id = phase_id_;
    while(try_add_next_phase(phase_id))
    {
        phase_id += 1;
    }

    // ops_phases_.push_back(phase_id);

    // if (!continuous_moving_list_.empty())
    // {
    //     auto const &cml = continuous_moving_list_;
    //     std::for_each(cml.rbegin(), cml.rend(), [this](auto const &phase) {
    //         ops_phases_.push_back(phase.phase_id);
    //     });
    //
    //     if (!continuous_moving_list_.back().axis_count)
    //         continuous_moving_list_.pop_back();
    // }

    if (continuous_moving_list_.empty() && phases_[phase_id_]->cmd() != VmPhaseType::GoTo)
        ops_phases_.push_back(ops_phases_.back() + 1);

    std::reverse(ops_phases_.begin(), ops_phases_.end()); 

    std::stringstream ss;
    for(std::size_t i = 0; i < ops_phases_.size(); ++i)
        ss << ((i == 0) ? "" : ", ") << ops_phases_[i];
    eng::log::info("OPS: [ {} ]", ss.view());

    return !continuous_moving_list_.empty();
}

// Поиск следующей позиции
bool vm::try_add_next_phase(std::size_t phase_id)
{
    // Находим следующию операцию с учетом циклов
    // dec_N_goto_pid_.clear();
    // dec_known_goto_pid_.clear();

    VmPhase const* vm_phase = get_next_operation(phase_id);

    // Если сейчас выполняется последний этап либо следующий этап не операция
    if (vm_phase != nullptr)
    {
        // Формируем следующую позицию для cnc согласно операции
        VmOperation const &item = *static_cast<VmOperation const*>(vm_phase);

        // Применяем только операции позиционирования
        std::ranges::for_each(item.items, [this, &item](VmOperation::Item *op)
        {
            auto it = map_pos_operations_.find(op->cmd());
            if (it == map_pos_operations_.end())
                return;
            // Заполняем next_target_position и next_phase_motion_speed_
            (this->*it->second)(op, item.absolute);
        });

        // Необходимо проверить что новая позиция не приводит к инверсии движения
        // Проверяем что следующая позиция не содержит инверсии движения
        auto it = std::ranges::find_if(phase_moving_, [this](auto &pair)
        {
            auto &axis = pair.second;

            int new_moving_direction = 0;
            if (axis.position != axis.next_position)
                new_moving_direction = (std::signbit(axis.next_position - axis.position) ? -1 : 1);

            // eng::log::info("FIND IF {}: md = {}, new-md = {}, pos = {}, next-pos = {}",
            //         pair.first, axis.moving_direction, new_moving_direction, axis.position, axis.next_position);

            // Направление движения в рамках одной оси должно сохраняться
            return axis.moving_direction && (axis.moving_direction != new_moving_direction);

            // if (axis.moving_direction == 0 || )
            //     return false;
            //
            // bool next_move = std::signbit(axis.next_position - axis.position);
            // bool prev_move = std::signbit(axis.moving_direction);
            // return next_move != prev_move;
        });

        // Если нашли инверсию, дальше не движемся
        if (it == phase_moving_.end())
        {
            // Добавляем запись в список будующих движений
            continuous_moving_list_.emplace_back(phase_id);
            auto &moving = continuous_moving_list_.back();

            moving.speed = next_phase_motion_speed_;
            phase_motion_speed_ = next_phase_motion_speed_;

            moving.axis_count = 0;

            std::ranges::for_each(phase_moving_, [this, &moving](auto &pair)
            {
                auto &axis = pair.second;

                if (axis.position == axis.next_position)
                    return;

                // Сохраняем пройденную дистанцию
                moving.distance[moving.axis_count] = { pair.first, (axis.next_position - axis.position) };
                moving.segments[moving.axis_count] = { pair.first, axis.position, axis.next_position };

                moving.axis_count += 1;
            });

            // На данном этапе нет изменения позиции
            // а значит движение отсутствует
            if (moving.axis_count != 0)
            {
                // Фиксируем новое состояние
                std::ranges::for_each(phase_moving_, [this](auto &pair)
                {
                    auto &axis = pair.second;

                    if (axis.position != axis.next_position)
                        axis.moving_direction = (std::signbit(axis.next_position - axis.position) ? -1 : 1);

                    axis.position = axis.next_position;
                });

                return true;
            }

            continuous_moving_list_.pop_back();
        }

        // // Откатываем назад циклы, которые были затронуты поиском данной операции
        // std::ranges::for_each(dec_N_goto_pid_, [this](std::size_t pid)
        // {
        //     eng::log::info("GOTO N REVERT: {}", pid);
        //     known_goto_[pid] += 1;
        // });
    }

    // // Откатываем назад удаленный цикл, которые были затронуты поиском данной операции
    // std::ranges::for_each(dec_known_goto_pid_, [this](std::size_t pid)
    // {
    //     eng::log::info("GOTO REVERT: {}", pid);
    //     known_goto_[pid] = 0;
    // });

    return false;
}

// Находим следующию операцию с учетом циклов
VmPhase const* vm::get_next_operation(std::size_t pid) noexcept
{
    if (pid == phases_.size())
    {
        ops_phases_.push_back(pid);
        return nullptr;
    }

    VmPhase const* phase = phases_[pid];
    if (phase->cmd() == VmPhaseType::Operation)
    {
        ops_phases_.push_back(pid);
        return phase;
    }

    if (phase->cmd() != VmPhaseType::GoTo)
    {
        ops_phases_.push_back(pid);
        return nullptr;
    }

    // Исследуем этап, на который ведет переход при условии что количество повторений еще осталось
    VmGoTo const &item = *static_cast<VmGoTo const*>(phase);

    auto it = known_goto_.find(pid);
    if (it == known_goto_.end())
    {
        eng::log::info("LOOP in phase-id = {} INIT BY: {}", pid, item.N);
        known_goto_[pid] = item.N;
    }

    // Операция перехода не актуальна, проверяем следующий за ней этап
    if (known_goto_[pid] == 0)
    {
        // Сохраняем использованный цикл этапа с переходом чтобы если что отменить их
        // dec_known_goto_pid_.push_back(pid);

        known_goto_.erase(pid);

        pid += 1;

        eng::log::info("LOOP DONE, GO TO: {}", pid);
        return get_next_operation(pid);
    }

    // Сохраняем номер этапа с переходом чтобы если что отменить их
    // dec_N_goto_pid_.push_back(pid);

    known_goto_[pid] -= 1;
    pid = item.phase_id;

    eng::log::info("GOTO: {}", pid);

    // Проверяем этап, на который указывает операция перехода
    return get_next_operation(pid);
}

// Проверяем, достигли ли мы ожидаемой позиции
bool vm::check_in_position(std::unordered_map<char, double> const &positions) const
{
    if (ops_phases_.empty() || continuous_moving_list_.empty())
        throw std::runtime_error("check_in_position: moving lists was empty");

    if (ops_phases_.empty() > continuous_moving_list_.empty())
        throw std::runtime_error("check_in_position: moving list less then ops phases");

    // std::stringstream ss;
    // ss << "[ ";
    // std::ranges::for_each(positions, [&ss](auto const &pair) {
    //     ss << std::format(" {}: {:.3f} ", pair.first, pair.second);
    // });
    // ss << " ]";
    // eng::log::info("check_in_position: {}", ss.view());

    std::size_t idx = (continuous_moving_list_.size() + 1) - ops_phases_.size();

    // Никогда не анализируем последную позицию в движении,
    // она стартует по факту завершения всех заданий движения
    if (idx + 1 == continuous_moving_list_.size())
        return false;

    // Берем текущий этап и проверяем, преодолели ли мы его
    auto const &pmi = continuous_moving_list_[idx];
    auto const end = pmi.segments + pmi.axis_count;

    // Ищем ту ось, которая еще не доехала
    auto it = std::find_if_not(pmi.segments, end,
            [this, &positions](auto const &segment)
            {
                double current_position = positions.find(segment.axis)->second;

                // Ось не выполняет движения
                if (segment.from == segment.to)
                {
                    // eng::log::info("\t{}: {:.3f} == {:.3f}", segment.axis, segment.from, segment.to);
                    return true;
                }

                // Если мы движемся в сторону увеличения
                if (segment.from < segment.to )
                {
                    // eng::log::info("\t{}: {:.3f} >= {:.3f}", segment.axis, current_position, segment.to);
                    return current_position >= segment.to;
                }

                // eng::log::info("\t{}: {:.3f} <= {:.3f}", segment.axis, current_position, segment.to);
                return current_position <= segment.to;
            });

    // eng::log::info("\t{}", (it == end) ? "OK" : "WAIT");

    // Если нашли хоть одну ось, которая еще не в координатах, ждем дальше
    // Иначе мы можем выполнить достигнутый этап
    return (it == end);
}

void vm::_vm_next_op_pos(VmOperation::Item const* item, bool absolute) noexcept
{
    VmOperation::Pos const& op = *static_cast<VmOperation::Pos const*>(item);
    auto &axis = phase_moving_[op.axis];

    if (absolute)
        axis.next_position = op.pos;
    else
        axis.next_position = axis.position + op.pos;
}

void vm::_vm_next_op_speed(VmOperation::Item const* item, bool) noexcept
{
    VmOperation::Speed const& op = *static_cast<VmOperation::Speed const*>(item);
    next_phase_motion_speed_ = op.speed;
}

// Создаем из внутреннего представления
// представление для передачи на выполнение
void vm::fill_cnc_task(eng::abc::pack &args) const
{
    // Количество этапов
    args.push<std::uint8_t>(continuous_moving_list_.size());

    std::ranges::for_each(continuous_moving_list_, [&args](auto const &phase)
    {
        // Скорость движения
        args.push<double>(phase.speed);

        // Количество осей в задании
        args.push<std::uint8_t>(phase.axis_count);

        // Имя и дистанция для каждой оси
        std::for_each(phase.distance, phase.distance + phase.axis_count,
            [&args](auto const &distance)
            {
                args.push<char>(distance.axis);
                args.push<double>(distance.value);
            });
    });
}

// Создаем описание состояния всей системы для
// его передачи на выполнение
// [ "fc", ... ]
// [ "center", ... ]

void vm::fill_stuff_task(std::size_t phase_id, eng::abc::pack &args) const
{
    VmPhase const *phase = phases_[phase_id];

    switch (phase->cmd())
    {
    case VmPhaseType::Operation:
        args.push("operation");
        fill_operation_task(*static_cast<VmOperation const*>(phase), args);
        break;

    default:
        break;
    }
}

// [ "operation", fc-i, fc-p, sp0, sp1, sp2, N, [ axis, speed, ... ] ]
void vm::fill_operation_task(VmOperation const &op, eng::abc::pack &args) const
{
    std::array<bool, 3> sprayers;
    std::vector<std::pair<char, double>> axis;

    double fc_i;
    double fc_p;

    for (VmOperation::Item *item : op.items)
    {
        switch(item->cmd())
        {
        case VmOpType::FC: {
            VmOperation::FC const& op = *static_cast<VmOperation::FC const *>(item);
            fc_p = op.power;
            fc_i = op.current;
            break;
        }

        case VmOpType::Spin: {
            VmOperation::Spin const& op = *static_cast<VmOperation::Spin const*>(item);
            axis.emplace_back(op.axis, op.speed);
            break;
        }

        case VmOpType::Sprayer: {
            VmOperation::Sprayer const& op = *static_cast<VmOperation::Sprayer const*>(item);
            sprayers[op.id] = op.state;
            break;
        }

        default:
            break;
        }
    }

    args.push(fc_i);
    args.push(fc_p);

    args.push(sprayers[0]);
    args.push(sprayers[1]);
    args.push(sprayers[2]);

    args.push<std::uint8_t>(axis.size());
    std::ranges::for_each(axis, [&args](auto const &axis)
    {
        args.push(axis.first);
        args.push(axis.second);
    });
}

