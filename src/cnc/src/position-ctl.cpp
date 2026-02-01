#include "position-ctl.hpp"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <numeric>

struct calculate_offset_t
{
    double time;
    double v;
    double acc = 0.0;
};

static constexpr double calculate_offset(calculate_offset_t args) noexcept
{
    double offset = std::abs(args.v) * args.time;
    offset += 0.5 * args.acc * args.time * args.time;
    return std::copysign(offset, args.v);
}

struct calculate_vmax_t
{
    double v0 = 0.0;
    double s;
    double acc_up;
    double acc_down;
};

// Считаем максимальную скорость из всего пути и ускарений
static constexpr double calculate_vmax(calculate_vmax_t args)
{
    // Определяем, сколько мы проедем за v0
    double t0 = args.v0 / args.acc_up;
    double s0 = (args.acc_up * t0 * t0) / 2.0;

    double numerator = 2 * (args.s + s0) * args.acc_up * args.acc_down;
    double denominator = args.acc_up + args.acc_down;
    return std::sqrt(numerator / denominator);
}

// Считаем скорость на определенном расстоянии при ускорений
static constexpr double calculate_v(double v0, double s, double acc)
{
    double value = v0 * v0 + 2 * acc * s;
    return std::sqrt(value);
}

// Считаем время достижения точки с ускорением 
static constexpr double calculate_time(double v0, double vmax, double acc)
{
    return (vmax - v0) / acc;
}

struct calculate_acc_phase_t
{
    double v0 = 0.0;
    double v;
    double acc;
};

static constexpr std::pair<double, double> calculate_acc_phase(calculate_acc_phase_t args)
{
    // Считаем время, за которое мы достигнем максимальной скорости с заданным ускорением
    double t0 = args.v0 / args.acc;
    double t1 = args.v / args.acc;
    // Пройденное за это время расстояние составит
    double s0 = (args.acc * t0 * t0) / 2.0;
    double s1 = (args.acc * t1 * t1) / 2.0;

    return { s1 - s0, t1 - t0 };
}

static constexpr std::pair<double, double> calculate_vmax_phase(double vmax, double len, double s0, double s2)
{
    // Отрезок пути с максимальной скоростью
    double s = (len > (s0 + s2)) ? (len - s0 - s2) : 0.0;
    // Тогда с постоянной максимальной скоростью мы будем двигаться
    double t = s / vmax;

    return { s, t };
}

static constexpr void reset_task(position_ctl::task_t &task)
{
    task.speed_up = { 0.0, 0.0 };
    task.vmax = { 0.0, 0.0 };
    task.speed_down = { 0.0, 0.0 };
}

static constexpr void print_task(position_ctl::task_t const &task)
{
    std::println("TASK: [{:.3f}][{:.3f}]", task.position, task.distance);

    std::println("\tUP  : v: {:.3f}, t: {:.3f}, s: {:.3f}", task.speed_up.v, task.speed_up.t, task.speed_up.s);
    std::println("\tVMAX: v: {:.3f}, t: {:.3f}, s: {:.3f}", task.vmax.v, task.vmax.t, task.vmax.s);
    std::println("\tDOWN: v: {:.3f}, t: {:.3f}, s: {:.3f}", task.speed_down.v, task.speed_down.t, task.speed_down.s);
}

position_ctl::position_ctl(char axis)
    : eng::sibus::node(std::format("position-ctl.{}", axis))
{
    node::add_task_handler()
        .on_attach([this](eng::abc::pack args)
        {
            std::println("position_ctl::on-attach");
            return eng::sibus::task_session_id_t{ 0 };
        })
        .on_detach([this](eng::sibus::task_session_id_t)
        {
            std::println("position_ctl::on-detach");
        })
        .on_start([this](eng::sibus::task_session_id_t, eng::abc::pack args)
        {
            std::println("position_ctl::on-start");
            start_motion(args);
        })
        .on_update([this](eng::sibus::task_session_id_t, eng::abc::pack args)
        {
            std::println("position_ctl::on-start");
        })
        .on_cancel([this](eng::sibus::task_session_id_t)
        {
            std::println("position_ctl::on-cancel");
        });

    port_mode_ = node::add_output_port("mode");
    port_position_ = node::add_output_port("position");
    // port_status_ = node::add_output_port("status");

    timer_id_ = eng::timer::create([this] {
        update_position();
    });

    node::set_port_value(port_mode_, "csp");
}

// 0 - Режим движения
//  V - скорость
//  S - коородината
// 1 - смысл значения
//  a - абсолютное
//  r - относительное
// 2 - значение
void position_ctl::start_motion(eng::abc::pack const &args)
{
    std::println("position_ctl::start_motion: {}", eng::utils::to_hex(args.to_span()));

    char mode = eng::abc::get<char>(args, 0);
    char vmode = eng::abc::get<char>(args, 1);
    double value = eng::abc::get<double>(args, 2);

    switch(mode)
    {
    case 'V':
        add_speed_task(vmode, value);
        return;
    case 'S':
        add_position_task(vmode, value);
        return;
    }

    std::println("position_ctl::start_motion: Неизвестный режим движения: {}", mode);


    //     char mode = eng::abc::get<char>(args, 0);
    //     double position = eng::abc::get<double>(args, 1);
    //     add_position_task(mode, position);
    //
    //     // char mode = 'a';//eng::abc::get<char>(args, 0);
    //     double speed = eng::abc::get<double>(args, 0);
    //     add_speed_task('a', speed);
}

void position_ctl::add_position_task(char mode, double position)
{
    // std::println("position_ctl::add_next_task[{}][{}]", mode, position);

    double target_position = position_;
    if (!tasks_.empty())
        target_position = tasks_.back().position;

    // Относительная позиция
    if (mode == 'r')
    {
        // Получаем последную позицию
        position += target_position;
    }
    else if (mode != 'a')
    {
        std::println("position_ctl::add_next_task: Задан неверный режим: {}", mode);
        return;
    }

    // Все расстояние, которое необходимо пройти
    double len = position - target_position;

    // Считаем максимальную скорость
    double vmaxmax = calculate_vmax({ .s = std::abs(len), .acc_up = speed_up_, .acc_down = speed_down_ });
    double vmax = std::min(vmaxmax, max_speed_);

    // Фаза разгона
    auto [ s0, t0 ] = calculate_acc_phase({ .v = vmax, .acc = speed_up_ });
    // Фаза торможения
    auto [ s2, t2 ] = calculate_acc_phase({ .v0 = vmax, .v = 0.0, .acc = -speed_down_ });
    // Отрезок пути с максимальной скоростью
    auto [ s1, t1 ] = calculate_vmax_phase(vmax, std::abs(len), s0, s2);

    // Запоминаем если мы первое задание
    bool first_task = tasks_.empty();

    tasks_.push_back({ position, len, { 0.0, t0, s0 }, { (t1 ? vmax : 0.0), t1, s1 }, { vmax, t2, s2 } });
    // std::println("--------------------------ADD NEW TASK-----------------------------");
    // print_task(tasks_.back());
    // std::println("-------------------------------------------------------------------");

    // Не оптимизируем если это первое задание
    if (!first_task)
    {
        optimize_movement();

        // std::println("-------------------------OPTIMIZED-----------------------------");
        // std::ranges::for_each(tasks_, [](auto const &t)
        // {
        //     print_task(t);
        // });
        // std::println("---------------------------------------------------------------");
    }
    else
    {
        // Инициируем таймер если это первое задание
        time_start_ = std::chrono::steady_clock::now();
        gtime_start_ = std::chrono::steady_clock::now();
        eng::timer::start(timer_id_, 100);
    }
}

// Регулируем вращение за счет измерения скорости
void position_ctl::add_speed_task(char mode, double speed)
{
    std::println("position_ctl::add_speed_task: {} = {}", mode, speed);

    // Получаем текущую скорость
    double v0 = 0.0;
    bool first_task = tasks_.empty();

    if (!first_task)
    {
        auto pti = get_process_task_info();
        if (pti.t < 0.0)
        {
            auto const &task = tasks_.front();

            // Время, прошедшее с начала выполнения данного задания
            double time = phase_time_elapsed();
            std::println("position_ctl::add_speed_task: time: {}", time);

            // Добрались до максимальной скорости
            if (time >= task.speed_up.t)
            {
                pti.v = task.vmax.v;
                std::println("position_ctl::add_speed_task: 2ph v0: {}", pti.v);
            }
            else
            {
                // Успели разогнаться до
                double s0 = calculate_offset({ .time = time, .v = task.speed_up.v, .acc = speed_up_ });
                pti.v = calculate_v(task.speed_up.v, s0, speed_up_);

                std::println("position_ctl::add_speed_task: 1ph s0: {}, v0: {}", s0, pti.v);

            }
        }
        v0 = std::copysign(pti.v, tasks_.front().distance);
        std::println("position_ctl::add_speed_task: v0: {}, pti.v: {}", v0, pti.v);
    }

    double vmax = speed;

    // Список текущих задач не актуален
    tasks_.clear();

    double position = NAN;
    double len = std::copysign(INFINITY, speed);

    // Направление движения не меняется
    if (std::signbit(v0) == std::signbit(vmax))
    {
        // Необходимо замедлиться
        if (v0 > vmax)
        {
            // Фаза торможения
            auto [ s2, t2 ] = calculate_acc_phase({ .v0 = std::abs(v0), .v = std::abs(vmax), .acc = -speed_down_ });
            tasks_.push_back({ position, len, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { v0, t2, s2 } });

            //  Уравняли скорости
            v0 = vmax;
        }

    }
    // При смене направления движения
    else
    {
        // Сначала тормозим до остановки
        auto [ s2, t2 ] = calculate_acc_phase({ .v0 = std::abs(v0), .v = 0.0, .acc = -speed_down_ });
        tasks_.push_back({ position, len, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { v0, t2, s2 } });
    }

    // Фаза разгона и фаза основного движения
    auto [ s0, t0 ] = calculate_acc_phase({ .v0 = std::abs(v0), .v = std::abs(vmax), .acc = speed_up_ });
    tasks_.push_back({ position, len, { v0, t0, s0 }, { std::abs(vmax), 0.0, 0.0 }, { 0.0, 0.0, 0.0 } });

    std::println("-------------------------SPEED-----------------------------");
    std::ranges::for_each(tasks_, [](auto const &t)
    {
        print_task(t);
    });
    std::println("---------------------------------------------------------------");

    if (first_task)
    {
        // Инициируем таймер если это первое задание
        time_start_ = std::chrono::steady_clock::now();
        gtime_start_ = std::chrono::steady_clock::now();
        eng::timer::start(timer_id_, 10);
    }
}

void position_ctl::remove_front_tasks(std::size_t count)
{
    while (count != 0 && !tasks_.empty())
    {
        tasks_.pop_front();
        count -= 1;
    }

    std::println("-------------------------LEFT TASKS-----------------------------");
    std::ranges::for_each(tasks_, [](auto const &t)
    {
        print_task(t);
    });
    std::println("---------------------------------------------------------------");
}

void position_ctl::shift_task_start_time(double time)
{
    std::chrono::duration<double> seconds(time);
    time_start_ += std::chrono::duration_cast<std::chrono::steady_clock::duration>(seconds);
}

position_ctl::task_phase_t position_ctl::get_process_task_info() const
{
    auto const &task = tasks_.front();

    // Время, прошедшее с начала выполнения данного задания
    double time = phase_time_elapsed();
    time -= (task.speed_up.t + task.vmax.t);
    std::println("position_ctl::get_process_task_info: time: {}", time);

    // Если мы преодолели первый и второй этапы и находимся на третьем
    // а значит начали снижать скорость, необходимо учесть данный факт при перерасчете
    if (time > 0.0)
    {
        // За это время мы пройдем вот такой путь
        double s0 = calculate_offset({ .time = task.speed_up.t, .v = task.speed_up.v, .acc = speed_up_ });
        double s1 = calculate_offset({ .time = task.vmax.t, .v = task.vmax.v });
        double s2 = calculate_offset({ .time = time, .v = task.speed_down.v, .acc = -speed_down_ });

        double sss = std::copysign(s0 + s1 + s2, task.distance);
        std::println("position_ctl::get_process_task_info: time: {}, s0: {}, s1: {}, s2: {}, sss: {}", time, s0, s1, s2, sss);

        // Успели скинуть скорость на значение
        return { .v = calculate_v(task.speed_down.v, s2, -speed_down_), .t = time, .s = sss };
    }

    //  Если есть этап разгона, берем начальную скорость оттуда
    double v0 = task.speed_up.t ? task.speed_up.v : task.vmax.v;
    return { .v = v0, .t = time, .s = 0.0 };
}

// Если это не первое задание, необходимо убрать остановку у
// предидущего задания, но только при условии что нам не необходимо
// двигаться в противоположном направлении
void position_ctl::optimize_movement()
{
    auto from = std::prev(tasks_.end());

    // Перемещаемся назад в поисках доступных для оптимизации задач
    while(from != tasks_.begin())
    {
        auto prev = std::prev(from);

        // Если скорость с разным знаком, нельзя оптимизировать
        if (std::signbit(from->distance) != std::signbit(prev->distance))
            break;

        // Если нет этапа торможения
        if (prev->speed_down.t == 0.0)
        {
            // и скорость на разгоне достигла максимума, нечего оптимизировать
            double vmax = calculate_v(prev->speed_up.v, std::abs(prev->distance), speed_up_);
            if (vmax >= max_speed_) break;
        }

        from = prev;
    }

    // Нечего оптимизировать
    if (from == std::prev(tasks_.end()))
        return;

    // Считаем общее расстояние всех этапов
    double distance = std::accumulate(from, tasks_.end(), 0,
        [](double acc, auto const &t) { return acc + t.distance; });
    distance = std::abs(distance);

    // Необходимо инициализировать скорость тем значением,
    // которое у нас будет после завершения предидущего этапа
    double v0 = 0.0;
    if (from != tasks_.begin())
    {
        // Получаем предидущую задачу
        auto task = std::prev(from);

        // Если у задачи есть этап с максимальной скорость, берем ее
        // иначе считаем с какой скоростью мы завершим этап разгона
        v0 = (task->vmax.t) ? task->vmax.v :
            calculate_v(task->speed_up.v, std::abs(task->distance), speed_up_);
    }
    // Если же мы на самом первом задании, он выполняется в данный момент
    // и мы не можем просто так его модифицировать, необходимо учесть
    // уже выполненные этапы
    else
    {
        auto pti = get_process_task_info();
        v0 = pti.v;

        // Если мы преодолели первый и второй этапы и находимся на третьем
        // а значит начали снижать скорость, необходимо учесть данный факт при перерасчете
        if (pti.t > 0.0)
        {
            // Отнимаем это от всего пути
            from->distance -= pti.s;

            // Отнимаем от общей дистанции рассчетной
            distance -= std::abs(pti.s);

            position_ += pti.s;

            shift_task_start_time(from->speed_up.t + from->vmax.t + pti.t);
        }
    }

    // Считаем отрезки для данного расстояния
    double vmaxmax = calculate_vmax({ .v0 = v0, .s = distance, .acc_up = speed_up_, .acc_down = speed_down_ });
    double vmax = std::min(vmaxmax, max_speed_);

    auto [ s0, t0 ] = calculate_acc_phase({ .v0 = v0, .v = vmax, .acc = speed_up_ });
    auto [ s2, t2 ] = calculate_acc_phase({ .v0 = vmax, .v = 0.0, .acc = -speed_down_ });
    auto [ s1, t1 ] = calculate_vmax_phase(vmax, distance, s0, s2);

    auto task = from;
    reset_task(*task);

    // Распределяем общую траекторию по задачам и их этапам
    while (s0 > 0.0 && task != tasks_.end())
    {
        task->speed_up.s = std::min(std::abs(task->distance), s0);
        // Начальная скорость
        task->speed_up.v = v0;

        // В конце задачи мы разгонимся до скорости
        double vmax = calculate_v(v0, task->speed_up.s, speed_up_);

        // При этом пройдет времени
        task->speed_up.t = calculate_time(v0, vmax, speed_up_);

        // Теперь стартовая скорость у следующего этапа будет другой
        v0 = vmax;

        // Данная задача целиком помещается в этап разгона
        if (std::abs(task->distance) > s0)
        {
            // Задача продолжается и на следующем этапе
            break;
        }

        // Отнимаем пройденное расстояние
        s0 -= task->speed_up.s;

        task = std::next(task);
        reset_task(*task);
    }

    // Распределяем расстояние, которое будет пройдено без ускорения
    while (s1 > 0.0 && task != tasks_.end())
    {
        task->vmax.s = std::min(std::abs(task->distance) - s0, s1);
        task->vmax.v = v0;
        task->vmax.t = task->vmax.s / v0;

        // Данная задача целиком помещается в этап движения без ускорения
        if (std::abs(task->distance) > s0 + s1)
        {
            // Задача продолжается и на следующем этапе
            break;
        }

        // Если текущая задача пришла из прошлого цикла

        s1 -= task->vmax.s;

        task = std::next(task);
        reset_task(*task);

        s0 = 0.0;
    }

    while (s2 > 0.0 && task != tasks_.end())
    {
        task->speed_down.s = std::min(std::abs(task->distance) - s0 - s1, s2);
        // Начальная скорость
        task->speed_down.v = v0;

        // В конце задачи мы разгонимся до скорости
        double vmin = calculate_v(v0, task->speed_down.s, -speed_down_);
        task->speed_down.t = calculate_time(v0, vmin, -speed_down_);

        // Теперь стартовая скорость у следующего этапа будет другой
        v0 = vmin;

        // Данная задача целиком помещается в этап замедления
        if (std::abs(task->distance) > s0 + s1 + s2)
        {
            // Задача продолжается и на следующем этапе
            break;
        }

        s2 -= task->speed_down.s;

        task = std::next(task);
        reset_task(*task);

        s0 = 0.0;
        s1 = 0.0;
    }
}

// Считаем сколько времени прошло с начала текущего этапа
double position_ctl::phase_time_elapsed() const noexcept
{
    auto time_diff = std::chrono::steady_clock::now() - time_start_;
    return std::chrono::duration<double, std::chrono::seconds::period>(time_diff).count();
}

// По таймеру обновляем позицию
void position_ctl::update_position()
{
    double time = phase_time_elapsed();

    double offset = get_phase_offset(time);
    std::int32_t result = static_cast<std::int32_t>(position_ + offset);

    node::set_port_value(port_position_, result);

    {
        auto time_diff = std::chrono::steady_clock::now() - gtime_start_;
        double gt = std::chrono::duration<double, std::chrono::seconds::period>(time_diff).count();
        std::println("{:.3f}\t{}", gt, result);
    }

    // Все задачи выполнены, останавливаем таймер
    if (tasks_.empty())
        eng::timer::stop(timer_id_);
}

double position_ctl::get_phase_offset(double time)
{
    if (tasks_.empty())
        return 0.0;

    auto const &t = tasks_.front();

    // Пройденное расстояние на данной задаче
    double offset = 0.0;

    // Мы разгоняемся
    if (time < t.speed_up.t)
    {
        // std::println("acceleration");
        double ds = calculate_offset({ .time = time, .v = t.speed_up.v, .acc = speed_up_ });
        return std::copysign(ds, t.distance);
    }

    // Мы движемся с постоянной скоростью
    time -= t.speed_up.t;
    offset += std::copysign(t.speed_up.s, t.distance);

    if (time < t.vmax.t)
    {
        // std::println("maximum speed");
        double ds = calculate_offset({ .time = time, .v = t.vmax.v, .acc = 0.0 });
        return offset + std::copysign(ds, t.distance);
    }

    // Мы тормозим
    time -= t.vmax.t;
    offset += std::copysign(t.vmax.s, t.distance);
    if (time < t.speed_down.t)
    {
        // std::println("decceleration");
        double ds = calculate_offset({ .time = time, .v = t.speed_down.v, .acc = -speed_down_ });
        return offset + std::copysign(ds, t.distance);
    }

    std::println("DONE");

    // Актуализируем ее как текущее новое состояние
    // системы и удаляем из очереди задачь

    position_ = t.position;

    offset += std::copysign(t.speed_down.s, t.distance);
    offset = t.distance - offset;

    shift_task_start_time(t.speed_up.t + t.vmax.t + t.speed_down.t);

    tasks_.pop_front();

    return offset;
}
