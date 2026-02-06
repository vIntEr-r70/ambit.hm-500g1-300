#include "multi-axis-ctl.hpp"
#include "common/axis-config.hpp"

#include <eng/json.hpp>
#include <eng/sibus/client.hpp>
#include <eng/timer.hpp>
#include <eng/log.hpp>

#include <algorithm>
#include <optional>
#include <sstream>
#include <numeric>
#include <filesystem>
#include <chrono>
#include <stdexcept>

template <typename T>
constexpr static double calc_distance(T const &m)
{
    double s0 = m.v0 * m.t_acc + m.a * m.t_acc * m.t_acc * 0.5;
    double s2 = m.v1 * m.t_dec + m.a * m.t_dec * m.t_dec * 0.5;
    double s1 = m.v * m.t_v;
    return s0 + s1 + s2;
}

template <typename T>
constexpr static void print_result(T const &phases)
{
    std::ranges::for_each(phases, [](auto const &pair)
    {
        auto const &axis = pair.second;

        eng::log::info("{}:", pair.first);
        std::ranges::for_each(axis.phases, [](auto const &phase)
        {
            eng::log::info("\tstart-time: {}", phase.start_time);
            eng::log::info("\tmove-step: {}", phase.move_step);
            std::stringstream ss;
            ss << "[ ";
            bool first = true;
            std::ranges::for_each(phase.moves, [&ss, &first](auto const &move)
            {
                ss << (first ? "" : ", ") << "\n";
                ss << std::format("dS: {:.3f}", calc_distance(move)) << "\n";
                ss << std::format("steps: [ ds = {:.3f}, v0 = {:.3f}, v = {:.3f}, v1 = {:.3f}, a = {:.3f}, t = {:.3f}, t0 = {:.3f}, t1 = {:.3f}, t2 = {:.3f} ]",
                        move.ds, move.v0, move.v, move.v1, move.a, (move.t_acc + move.t_dec + move.t_v), move.t_acc, move.t_v, move.t_dec);
                first = false;
            });
            ss << " ]";
            eng::log::info("\nmove: {}", ss.view());
        });
    });
}

multi_axis_ctl::multi_axis_ctl()
    : eng::sibus::node("multi-axis-ctl")
{
    ictl_ = node::add_input_wire();

    node::set_activate_handler(ictl_, [this](eng::abc::pack args) {
        activate(std::move(args));
    });
    node::set_deactivate_handler(ictl_, [this] {
        deactivate();
    });

    char const *LIAEM_RO_PATH = std::getenv("LIAEM_RO_PATH");
    if (LIAEM_RO_PATH == nullptr)
    {
        eng::log::error("{}: LIAEM_RO_PATH not set", name());
        return;
    }

    std::filesystem::path path(LIAEM_RO_PATH);
    path /= "axis.json";

    try
    {
        eng::json::array cfg(path);
        cfg.for_each([this](std::size_t, eng::json::value v)
        {
            eng::json::object obj(v);
            char axis = obj.get_sv("axis")[0];
            info_[axis].acc = 0.0;

            std::string key = std::format("axis/{}", axis);
            eng::sibus::client::config_listener(key, [this, axis](std::string_view json)
            {
                if (json.empty())
                    return;

                axis_config::desc desc;
                axis_config::load(desc, json);

                info_[axis].acc = desc.acc;

                eng::log::info("{}: {}: acc = {}", name(), axis, desc.acc);
            });
        });
    }
    catch(std::exception const &e)
    {
        eng::log::error("{}: {}", name(), e.what());
        return;
    }

    // Создаем узлы управления с драйверами шаговых двигателей
    // std::ranges::for_each(info_, [this](auto &info)
    // {
    //     char axis = info.first;
    //     auto ctl = node::add_output_wire(std::string(1, axis));
    //     info.second.ctl = ctl;
    //
    //     node::set_wire_status_handler(ctl, [this,axis] {
    //         wire_status_was_changed(axis);
    //     });
    //
    //     node::link_wires(ictl_, ctl);
    // });
}

void multi_axis_ctl::activate(eng::abc::pack args)
{
    tasks_.clear();
    in_proc_.clear();

    create_moving_tasks(std::move(args));

    // Получаем все оси, которые задействованы в программе
    help_set_.clear();
    std::ranges::for_each(tasks_, [this](auto const &item)
    {
        for (std::size_t i = 0; i < item.axis_count; ++i)
            help_set_.insert(item.axis[i].axis);
    });

    // Проверяем готовность осей, задействованных в задании
    auto it = std::ranges::find_if(help_set_, [this](char axis) {
        return info_.contains(axis) && !node::is_ready(info_[axis].ctl);
    });

    if (it != help_set_.end())
    {
        node::terminate(ictl_, "Присутствуют оси, не готовые к движению");
        return;
    }

    // Формируем список задания для осей
    convert_tasks_to_phases();

    // Отдаем на выполнение
    std::ranges::for_each(axis_phases_, [this](auto const &pair)
    {
        execute_phase(pair.first);
    });
}

void multi_axis_ctl::deactivate()
{
    if (in_proc_.empty())
    {
        node::set_ready(ictl_);
        return;
    }

    tasks_.clear();
}

// Рассчитываем для каждой оси время выполнения движения
void multi_axis_ctl::execute_phase(char axis)
{
    std::size_t phase_id = 0;

    // Если ось уже выполнялась то в списке
    // лежит номер следующего этапа
    if (in_proc_.contains(axis))
        phase_id = in_proc_[axis];

    auto const &phases = axis_phases_[axis].phases;
    if (phase_id == phases.size())
    {
        // Все этапы выполнены, удаляем ось из списка
        // и больше ничего ей не отправляем
        in_proc_.erase(axis);
        eng::log::info("{}: {}: DONE", name(), axis);

        if (in_proc_.empty())
        {
            // Все задания выполнены,
            // отправляем ответ нашему контроллеру
            eng::log::info("{}: ALL AXIS DONE", name());

            node::set_ready(ictl_);
        }

        return;
    }

    // Этап, требующий выполнения
    auto const &phase = phases[phase_id];

    // Сохраняем следующий для выполнения этап
    in_proc_[axis] += 1;

    // Формируем задание
    eng::abc::pack args;

    // Команда движения
    args.push("timed-shift");
    // время начала выполнения
    args.push(phase.start_time);
    // количество шагов движения
    args.push<std::uint8_t>(phase.moves.size());

    std::ranges::for_each(phase.moves, [&args](auto const &move)
    {
        // ускорение
        args.push(move.a);
        // t1, t2, t3
        args.push(move.t_acc);
        args.push(move.t_v);
        args.push(move.t_dec);
        // v, v1
        args.push(move.v);
        args.push(move.v1);
        // s
        args.push(move.ds);
    });

    eng::log::info("{}: {}: SEND: {}", name(), axis, args.size());

    // node::send_wire_signal(info_[axis].ctl, std::move(args));
    node::activate(info_[axis].ctl, std::move(args));
}

// Делим оси на 2 категории, оси со связанным движением и на самостоятельные
// связанные: те, которые стартуют и останавливаются вместе
// остальные: те, которые не удовлетворяют критерия связности
std::size_t multi_axis_ctl::fill_common_axis_list()
{
    axis_phases_.clear();
    axis_groups_.clear();

    std::size_t move_step = 0;

    // Заполняем список этапов для каждой оси
    std::ranges::for_each(tasks_, [&](auto const &task)
    {
        // Заполняем вспомогательный список имеющимися осями
        help_set_.clear();
        for (auto const & [key, _ ] : axis_phases_)
            help_set_.insert(key);

        // Для всех осей движения формируем описание
        std::for_each(task.axis, task.axis + task.axis_count,
            [&](auto &axis)
            {
                // Удаляем из вспомогательного списка имеющуюся ось
                help_set_.erase(axis.axis);

                // Создаем или получаем информацию об оси
                auto &list = axis_phases_[axis.axis];

                // Если ось не движется, создаем новый этап с
                // указанием времени начала движения
                if (list.stopped)
                {
                    list.stopped = false;
                    list.phases.emplace_back(move_step);
                }

                // Создаем запись о движении
                auto &phase = list.phases.back();
                phase.moves.emplace_back();
                auto &move = phase.moves.back();
                move.ds = axis.distance;
                move.v_total = task.speed;
            });

        move_step += 1;

        // Движения всех оставшиеся во вспомогательном списке осей
        // не являются непрерывными, говорим что она остановлена
        std::ranges::for_each(help_set_, [this](char axis) {
            axis_phases_[axis].stopped = true;
        });
    });


    // Формируем списки связанных осей по этапам
    // Для этого нам необходимо знать максимальное количество этапов
    for (std::size_t i = 0; i < move_step; ++i)
    {
        help_set_.clear();
        for (auto const & [key, _ ] : axis_phases_)
            help_set_.insert(key);

        auto iaxis = help_set_.begin();
        while (iaxis != help_set_.end())
        {
            // Если не стартует с нужного нам шага, удаляем
            if (get_phase_id(*iaxis, i).has_value())
                iaxis = std::next(iaxis);
            else
                iaxis = help_set_.erase(iaxis);
        }

        axis_groups_.emplace_back();
        while (!help_set_.empty())
        {
            // Остались те что стартуют, проверяем их длинну, она должна совпадать
            iaxis = help_set_.begin();
            auto phase_id = get_phase_id(*iaxis, i);
            std::size_t count = axis_phases_[*iaxis].phases[*phase_id].moves.size();

            axis_groups_.back().emplace_back(count);
            axis_groups_.back().back().axis.emplace_back(*iaxis, *phase_id);

            iaxis = help_set_.erase(iaxis);
            while (iaxis != help_set_.end())
            {
                auto phase_id = get_phase_id(*iaxis, i);
                if (count == axis_phases_[*iaxis].phases[*phase_id].moves.size())
                {
                    axis_groups_.back().back().axis.emplace_back(*iaxis, *phase_id);
                    iaxis = help_set_.erase(iaxis);
                }
                else
                {
                    iaxis = std::next(iaxis);
                }
            }
        }
        if (axis_groups_.back().empty())
            axis_groups_.pop_back();
    }

    std::ranges::for_each(axis_groups_, [](auto const &phases) {
        eng::log::info("Items in one phase");
        std::ranges::for_each(phases, [](auto const &axis) {
            eng::log::info("\tLinked axis: steps = {}", axis.moving_steps);
            std::ranges::for_each(axis.axis, [](auto const &axis) {
                eng::log::info("\t\t{}: phase-id = {}", axis.axis, axis.phase_id);
            });
        });
    });

    return move_step;
}

// Рассчитываем и заполняем параметры движения
// Синхронное движение группы осей
void multi_axis_ctl::calculate_moving(axis_group_t const &axis)
{
    eng::log::info("calculate_moving: moving-steps = {}", axis.moving_steps);

    // Сначала рассчитываем предельные скорость и ускорение для всех шагов
    for (std::size_t i = 0; i < axis.moving_steps; ++i)
    {
        // Считаем полную длинну пути
        double sum = std::accumulate(axis.axis.begin(), axis.axis.end(), 0.0,
                [this, i](double sum, axis_group_item_t axis)
                {
                    auto const &move = axis_phases_[axis.axis].phases[axis.phase_id].moves[i];
                    return sum + move.ds * move.ds;
                });

        double L = std::sqrt(sum);

        // Считаем скорости на каждом шаге движения
        std::ranges::for_each(axis.axis, [this, L, i](axis_group_item_t item)
        {
            auto &move = axis_phases_[item.axis].phases[item.phase_id].moves[i];
            // Граничная скорость движения
            move.k = std::abs(move.ds) / L;
            move.v_limit = move.v_total * move.k;
            move.v = move.v_limit;
            move.v0 = 0.0;
            move.v1 = 0.0;

            std::size_t gi = axis_phases_[item.axis].phases[item.phase_id].move_step + i;
            eng::log::info("\t[{}][{}][{}]: phase-id = {}, total = {}, limit = {}, v = {}",
                    i, gi, item.axis, item.phase_id, move.v_total, move.v_limit, move.v);
        });

        help_set_.clear();
        for (auto const & [axis, _ ] : axis.axis)
            help_set_.insert(axis);

        for (;;)
        {
            std::vector<axis_group_item_t>::const_iterator iaxis;

            // Если в списке остался только один претендент
            if (help_set_.size() > 1)
            {
                // Ищем ось с максимальной дистанцией
                iaxis = std::ranges::max_element(axis.axis, [&](auto const &a, auto const &b)
                {
                    if (!help_set_.contains(a.axis))
                        return false;

                    if (!help_set_.contains(b.axis))
                        return false;

                    auto const &move_a = axis_phases_[a.axis].phases[a.phase_id].moves[i];
                    auto const &move_b = axis_phases_[b.axis].phases[b.phase_id].moves[i];

                    return std::abs(move_a.ds) < std::abs(move_b.ds);
                });
            }
            else
            {
                // Берем первый из списка
                iaxis = std::ranges::find_if(axis.axis, [this](auto const &a) {
                    return a.axis == *help_set_.begin();
                });
            }

            char axis_a_total = iaxis->axis;
            double a_total = info_[axis_a_total].acc;
            double k_total = axis_phases_[axis_a_total].phases[iaxis->phase_id].moves[i].k;
            axis_phases_[axis_a_total].phases[iaxis->phase_id].moves[i].a = a_total;

            // Считаем ускорения
            std::ranges::for_each(axis.axis, [&](axis_group_item_t item)
            {
                if (item.axis == axis_a_total)
                    return;
                auto &move = axis_phases_[item.axis].phases[item.phase_id].moves[i];
                // move.a = a_total * (move.k * move.k) / (k_total * k_total);
                move.a = a_total * move.k / k_total;
            });

            // Ищем ось, у которое рассчетное ускорение превышает ее допустимое ускорение
            iaxis = std::ranges::find_if(axis.axis, [this, i](auto item)
            {
                auto &move = axis_phases_[item.axis].phases[item.phase_id].moves[i];
                return move.a > info_[item.axis].acc;
            });

            // Если таких нету значит мы все посчитали правильно
            if (iaxis == axis.axis.end())
                break;

            // Удаляем из списка кондидатов ось
            help_set_.erase(axis_a_total);

            if (help_set_.empty())
                throw std::runtime_error("bad acceleration search algorithm 2");
        }
    }
}

// Рассчитываем и заполняем параметры движения
// Независимое движение оси
void multi_axis_ctl::calculate_moving(axis_group_item_t item, std::size_t moving_steps)
{
    eng::log::info("{}: calculate_moving: moving-steps = {}, phase-id = {}", item.axis, moving_steps, item.phase_id);

    for (std::size_t i = 0; i < moving_steps; ++i)
    {
        // Считаем скорости на каждом шаге движения
        auto &axis_phase = axis_phases_[item.axis];
        auto &move = axis_phase.phases[item.phase_id].moves[i];
        move.v_limit = move.v_total;
        move.v = move.v_limit;
        move.a = info_[item.axis].acc;
        move.v0 = 0.0;
        move.v1 = 0.0;

        std::size_t gi = axis_phase.phases[item.phase_id].move_step + i;
        eng::log::info("\t[{}][{}]: total = {}, limit = {}, a = {}, v = {}",
                i, gi, move.v_total, move.v_limit, move.a, move.v);
    }
}

double multi_axis_ctl::phase_time_begin(std::size_t idx) const noexcept
{
    double sum = 0.0;
    for (std::size_t i = 0; i < idx; ++i)
        sum += move_step_times_[i];
    return sum;
}

std::optional<std::size_t> multi_axis_ctl::get_phase_id(char axis, std::size_t move_step) const noexcept
{
    auto const &phases = axis_phases_.find(axis)->second.phases;
    auto it = std::ranges::find_if(phases, [move_step](auto const &phase) {
        return move_step == phase.move_step;
    });

    if (it != phases.end())
        return std::distance(phases.begin(), it);

    return std::nullopt;
}

std::optional<std::size_t> multi_axis_ctl::get_phase_id_v2(char axis, std::size_t move_step) const noexcept
{
    auto const &phases = axis_phases_.find(axis)->second.phases;
    auto it = std::ranges::find_if(phases, [move_step](auto const &phase)
    {
        if (move_step < phase.move_step)
            return false;
        std::size_t idx = move_step - phase.move_step;
        return idx < phase.moves.size();
    });

    if (it != phases.end())
        return std::distance(phases.begin(), it);

    return std::nullopt;
}

std::size_t multi_axis_ctl::calculate_move_step(std::size_t istep)
{
    eng::log::info("Шаг N{}", istep);

    // Заполняем список осями, которые присутствуют на данном шаге
    help_phases_.clear();
    for (auto const & [axis, _ ] : axis_phases_)
    {
        auto phase_id = get_phase_id_v2(axis, istep);
        if (phase_id.has_value())
            help_phases_[axis] = *phase_id;
    }

    // Такого не может быть
    if (help_phases_.empty())
        throw std::runtime_error(std::format("empty move step detected: {}", istep));

    // Определяем синхронные оси
    help_axis_group_.clear();
    auto iaxis = help_phases_.begin();
    while (!help_phases_.empty())
    {
        char axis = iaxis->first;
        std::size_t phase_id = iaxis->second;

        auto &phase = axis_phases_[axis].phases[phase_id];
        auto &move = phase.moves[istep - phase.move_step];

        help_axis_group_.emplace_back(axis, phase_id);
        iaxis = help_phases_.erase(iaxis);

        while (iaxis != help_phases_.end())
        {
            auto const &iphase = axis_phases_[iaxis->first].phases[iaxis->second];
            if (iphase.move_step == phase.move_step && iphase.moves.size() == phase.moves.size())
            {
                help_axis_group_.emplace_back(iaxis->first, iaxis->second);
                iaxis = help_phases_.erase(iaxis);
            }
            else
            {
                iaxis = std::next(iaxis);
            }
        }

        eng::log::info("\tОсь {}{}", axis, (help_axis_group_.size() > 1) ? "+" : "");

        // Имеем группу из нескольких либо одной осей
        // выполняем рассчет первой оси из группы
        std::size_t next_step = calculate_axis_move_step(istep, move);
        // Производим откат либо на предидущий шаг либо начинаем с начата текущий
        if (next_step <= istep)
            return next_step;

        if (help_axis_group_.size() > 1)
        {
            // Считаем остальные оси в группе на основании рассчитанной оси
            for (std::size_t i = 1; i < help_axis_group_.size(); ++i)
            {
                auto const &gaxis = help_axis_group_[i];
                auto &phase = axis_phases_[gaxis.axis].phases[gaxis.phase_id];
                auto &gmove = phase.moves[istep - phase.move_step];

                eng::log::info("\tОсь {}+", gaxis.axis);

                std::size_t next_step = calculate_group_axis_move_step(istep, move, gmove);
                if (next_step <= istep)
                    return next_step;
            }
        }

        help_axis_group_.clear();
        iaxis = help_phases_.begin();
    }

    eng::log::info("DONE");

    // Можно двигаться дальше
    return istep + 1;
}

constexpr static std::optional<double> calculate_vmax(double S, double T, double a, double v0, double v1)
{
    double b = a * T + v0 + v1;
    double c = a * S + (v0 * v0 + v1 * v1) * 0.5;

    double discriminant = (b * b * 0.25) - c;

    if (discriminant < 0.0 && discriminant > -0.00000000001)
        discriminant = 0.0;

    // Если времени недостаточно чтобы преодолеть
    // указанное расстояние с указанным ускорением
    if (discriminant < 0)
        return std::nullopt;

    return (b * 0.5) - std::sqrt(discriminant);
}

constexpr static std::optional<double> calculate_min_time(double S, double a, double v0, double v1)
{
    // 1. Сначала считаем пиковую скорость для треугольного профиля
    double v_max_sq = a * S + (v0 * v0 + v1 * v1) * 0.5;

    // Проверка на физическую корректность (S должно быть достаточно для v0 и v1)
    // Ситуация, когда начальная или конечная скорость выше пиковой 
    // (нужно только торможение или только разгон)
    if (v_max_sq < (std::max(v0, v1) * std::max(v0, v1)))
        return std::nullopt;

    double v_max = std::sqrt(v_max_sq);

    // 2. Время — это сумма интервалов изменения скоростей
    return (2.0 * v_max - v0 - v1) / a;
}

constexpr static double calculate_max_v1(double S, double a, double v0)
{
    return std::sqrt(v0 * v0 + 2 * a * S);
}

// Для каждой оси на каждом шаге всегда посчитаны
// - ускорение
// - начальная скорость
// - конечная скорость
// Данные параметры в процессе согласования могут только уменьшаться
std::size_t multi_axis_ctl::calculate_axis_move_step(std::size_t istep, move_t &move)
{
    // Рассчетная длительность текущего шага
    double dt = move_step_times_[istep];

    eng::log::info("\t\tИсходное время выполнения: {:.3f}", dt);

    // Пробуем рассчитать максимальную скорость на данном шаге
    auto vmax = calculate_vmax(std::abs(move.ds), dt, move.a, move.v0, move.v1);

    // Если не удалось рассчитать значение, значит не достаточно времени
    if (!vmax)
    {
        // Пробуем получить минимальное необходимое время при имеющихся параметрах
        auto tmin = calculate_min_time(std::abs(move.ds), move.a, move.v0, move.v1);
        if (!tmin)
            throw std::runtime_error("плохие значения начальной и конечной скоростей");

        eng::log::info("\t\tРазница: {}", *tmin - dt);

        move_step_times_[istep] = *tmin;

        eng::log::info("\t\tНовое время выполнения: {:.3f}", *tmin);
        eng::log::info("\t\tНачинаем данный шаг заново...");

        // Остаемся на том же шаге пересчитываем остальные оси
        return istep;
    }
    else
    {
        move.v = *vmax;

        if (move.v > move.v_limit)
        {
            eng::log::info("\t\tНовое значение скорости: {:.3f} -> {:.3f}", move.v, move.v_limit);

            move.v = move.v_limit;

            // Время разгона
            move.t_acc = std::abs(move.v - move.v0) / move.a;
            // Время торможения
            move.t_dec = std::abs(move.v - move.v1) / move.a;

            double ds = std::abs(move.ds);
            ds -= (move.t_acc * move.t_acc * move.a * 0.5);
            ds -= (move.t_dec * move.t_dec * move.a * 0.5);

            // Время равномерного движения
            move.t_v = (ds / move.v) + 0.000000001;

            move_step_times_[istep] = move.t_acc + move.t_dec + move.t_v;

            eng::log::info("\t\tНовое время выполнения: {:.3f}", move_step_times_[istep]);
            eng::log::info("\t\tНачинаем данный шаг заново...");

            // Остаемся на том же шаге пересчитываем остальные оси
            return istep;
        }
    }

    // Время разгона
    move.t_acc = std::abs(move.v - move.v0) / move.a;
    // Время торможения
    move.t_dec = std::abs(move.v - move.v1) / move.a;
    // Время равномерного движения
    move.t_v = dt - move.t_acc - move.t_dec;

    eng::log::info("\t\tS: {:.3f}", calc_distance(move));
    eng::log::info("\t\tds: {:.3f}", move.ds);
    eng::log::info("\t\tv: {:.3f}", move.v);
    eng::log::info("\t\ta: {:.3f}", move.a);
    eng::log::info("\t\tk: {:.3f}", move.k);
    // eng::log::info("\t\tv0: {:.3f}", move.v0);
    eng::log::info("\t\tt_acc: {:.3f}", move.t_acc);
    eng::log::info("\t\tt_v  : {:.3f}", move.t_v);
    // eng::log::info("\t\tv1: {:.3f}", move.v1);
    eng::log::info("\t\tt_dec: {:.3f}", move.t_dec);

    // Можно двигаться дальше
    return istep + 1;
}

std::size_t multi_axis_ctl::calculate_group_axis_move_step(std::size_t istep, move_t const &base, move_t &move)
{
    move.v = base.v * move.k / base.k;
    // move.v0 = base.v0 * move.k / base.k;
    // move.v1 = base.v1 * move.k / base.k;

    if (move.v > move.v_limit)
    {
        eng::log::info("\t\tНовое значение скорости: {:.3f} -> {:.3f}", move.v, move.v_limit);

        move.v = move.v_limit;

        // Время разгона
        move.t_acc = std::abs(move.v - move.v0) / move.a;
        // Время торможения
        move.t_dec = std::abs(move.v - move.v1) / move.a;

        double ds = std::abs(move.ds);
        ds -= (move.t_acc * move.t_acc * move.a * 0.5);
        ds -= (move.t_dec * move.t_dec * move.a * 0.5);

        // Время равномерного движения
        move.t_v = ds / move.v;

        move_step_times_[istep] = move.t_acc + move.t_dec + move.t_v;

        eng::log::info("\t\tНовое время выполнения: {:.3f}", move_step_times_[istep]);
        eng::log::info("\t\tНачинаем данный шаг заново...");

        // Остаемся на том же шаге пересчитываем остальные оси
        return istep;
    }

    move.t_acc = base.t_acc;
    move.t_dec = base.t_dec;
    move.t_v = base.t_v;

    eng::log::info("\t\tS: {:.3f} {:.3f}", calc_distance(move), std::abs(base.ds) * move.k / base.k);
    eng::log::info("\t\tds: {:.3f}", move.ds);
    eng::log::info("\t\tv: {:.3f}", move.v);
    eng::log::info("\t\ta: {:.3f}", move.a);
    eng::log::info("\t\tk: {:.3f}", move.k);
    // eng::log::info("\t\tv0: {:.3f}", move.v0);
    eng::log::info("\t\tt_acc: {:.3f}", move.t_acc);
    eng::log::info("\t\tt_v  : {:.3f}", move.t_v);
    // eng::log::info("\t\tv1: {:.3f}", move.v1);
    eng::log::info("\t\tt_dec: {:.3f}", move.t_dec);

    return istep + 1;
}

// Запускаем цикл выполнения движения по нескольким осям
void multi_axis_ctl::register_on_bus_done()
{
    node::set_ready(ictl_);
}

// Ось завершила выполнение движения либо вернула ошибку
void multi_axis_ctl::response_handler(char axis, bool success)
{
    // Если хоть одна из осей вернула ошибку,
    // прекращаем движение остальных осей
    // Возвращаем результат выполнения задания
    if (!success) return;

    eng::log::info("{}: {}: {}", name(), axis,
        "1: Отправляем на выполнение следующий этап если таковой имеется");

    execute_phase(axis);
}

void multi_axis_ctl::wire_status_was_changed(char axis)
{
    // Если ось не выполняется, значит ее состояние нам не интересно
    if (!in_proc_.contains(axis))
        return;

    if (node::is_transiting(info_[axis].ctl))
        return;

    if (!tasks_.empty())
    {
        // Ось начала выполнять движение
        if (node::is_active(info_[axis].ctl))
            return;

        // Ось успешно завершила выполнять движение
        if (node::is_ready(info_[axis].ctl))
        {
            eng::log::info("{}: {}: {}", name(), axis,
                "2: Отправляем на выполнение следующий этап если таковой имеется");

            execute_phase(axis);
            return;
        }

        // Что-то произошло и необходимо деактивировать все активированныe оси
        // В качестве маркера того что мы ожидаем завершения задания

        tasks_.clear();

        std::ranges::for_each(in_proc_, [this, axis](auto const &pair)
        {
            // Пропускаем проблемную ось
            if (axis != pair.first)
            {
                if (node::is_active(info_[axis].ctl))
                {
                    node::deactivate(info_[axis].ctl);
                    return;
                }
            }

            // Удаляем оси, которые нам ждать нет необходимости
            in_proc_.erase(axis);
        });
    }
    else
    {
        if (node::is_active(info_[axis].ctl))
        {
            node::deactivate(info_[axis].ctl);
            return;
        }

        in_proc_.erase(axis);
    }

    if (in_proc_.empty())
        node::set_ready(ictl_);
}

// Np, [ speed, Na, [ axis, distance ], ... ], ... 
void multi_axis_ctl::create_moving_tasks(eng::abc::pack args)
{
    eng::log::info("multi_axis_ctl::create_moving_tasks");

    std::size_t iarg = 0;

    std::uint8_t n_phases = eng::abc::get<std::uint8_t>(args, iarg++);
    for (std::uint8_t i = 0; i < n_phases; ++i)
    {
        tasks_.emplace_back();
        auto &task = tasks_.back();

        task.speed = eng::abc::get<double>(args, iarg++);

        task.axis_count = eng::abc::get<std::uint8_t>(args, iarg++);
        for (std::uint8_t i = 0; i < task.axis_count; ++i)
        {
            task.axis[i] = {
                eng::abc::get<char>(args, iarg++),
                eng::abc::get<double>(args, iarg++)
            };
        }
    }
}

void multi_axis_ctl::convert_tasks_to_phases()
{
    // Разбиваем движение на этапы и группы по осям
    std::size_t move_steps = fill_common_axis_list();

    move_step_times_.resize(move_steps);
    std::ranges::fill(move_step_times_, 0.0);

    // Считаем движения каждой из групп
    std::ranges::for_each(axis_groups_, [this](auto &phases)
    {
        std::ranges::for_each(phases, [this](auto &group)
        {
            if (group.axis.size() == 1)
                calculate_moving(group.axis[0], group.moving_steps);
            else
                calculate_moving(group);
        });
    });

    std::size_t istep = 0;
    for (;;)
    {
        istep = calculate_move_step(istep);
        if (istep == move_steps)
            break;
    }

    // Инициализируем начальное время старта движения текущим временем
    auto now_time = std::chrono::steady_clock::now();
    double t = std::chrono::duration<double>(now_time.time_since_epoch()).count();
    // Смещаем на 10 мс в будующее
    t += 0.01;

    // Осталось распределить время по этапам
    std::ranges::for_each(axis_phases_, [this, t](auto &pair)
    {
        std::ranges::for_each(pair.second.phases, [this, t](auto &phase) {
            phase.start_time = t + phase_time_begin(phase.move_step);
        });
    });
}


