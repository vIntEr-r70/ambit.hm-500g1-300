#include "lock-intervals.hpp"
#include "common/lockers.hpp"

#include <eng/json.hpp>
#include <eng/log.hpp>
#include <eng/sibus/client.hpp>

#include <algorithm>
#include <bitset>

lock_intervals::lock_intervals(std::filesystem::path const &path)
    : eng::sibus::node("lock-intervals")
{
    if (!ambit::lock_system::load_config(path / "intervals.json", lockers_, subsystems_))
        return;

    for (std::size_t idx = 0; idx < lockers_.size(); ++idx)
    {
        auto &locker = lockers_[idx];

        // Для каждого контроллируемого параметра создаем входной порт
        // для получения измеренного значения
        node::add_input_port(locker.name, [this, idx](eng::abc::pack args)
        {
            // Обновляем состояние элемента
            update_locker(idx, args ? eng::abc::get<double>(args) : NAN);
        });

        locker.port = node::add_output_port(locker.name);

        // Подписываемся на конфигурацию данного параметра
        std::string key = std::format("locker/intervals/{}", locker.name);
        eng::sibus::client::config_listener(key, [this, idx](std::string_view json)
        {
            if (json.empty()) return;

            eng::json::object cfg(json);
            eng::json::array arr = cfg.get_array("value");

            auto &locker = lockers_[idx];
            for (std::size_t i = 0; i < 4; ++i)
                locker.range[i] = arr[i].get<double>();

            ambit::lock_system::configure_relationship(idx, cfg.get<std::uint32_t>("mask"), lockers_, subsystems_);
            update_locker(idx, locker.value);
        });
    }

    // Создаем выходные порты для выдачи статусов систем
    std::ranges::for_each(subsystems_, [this](auto &ss)
    {
        ss.port = node::add_output_port(ss.name);
    });

    for (std::size_t idx = 0; idx < lockers_.size(); ++idx)
        update_locker(idx, NAN);
}

static constexpr bool is_in_range(double value, double min, double max) noexcept {
    return (std::isnan(min) || (value > min)) && (std::isnan(max) || (value < max));
};

void lock_intervals::update_locker(std::size_t idx, double value)
{
    auto &ll = lockers_[idx];

    // Запоминаем последнее значение
    ll.value = value;

    if (std::isnan(value))
    {
        update_locker_status(idx, ll.subsystems.empty() ? ambit::lock_status::normal : ambit::lock_status::critical);
        return;
    }

    auto result = ambit::lock_status::normal;
    if (!ll.subsystems.empty())
    {
        if (!is_in_range(value, ll.range[1], ll.range[2]))
            result = ambit::lock_status::warning;

        if (!is_in_range(value, ll.range[0], ll.range[3]))
            result = ambit::lock_status::critical;
    }

    update_locker_status(idx, result);
}

void lock_intervals::update_locker_status(std::size_t idx, ambit::lock_status value)
{
    auto &ll = lockers_[idx];

    // Состояние не изменилось
    if (ll.status && (*ll.status == value))
        return;
    ll.status = value;

    node::set_port_value(ll.port, { static_cast<std::uint8_t>(*ll.status) });
    eng::log::info("{}: {}", ll.name, static_cast<std::uint8_t>(*ll.status));

    // Проходим по списку всех зависимых систем
    std::ranges::for_each(ll.subsystems, [this](std::size_t idx)
    {
        if (!ambit::lock_system::update_subsystem(idx, lockers_, subsystems_))
            return;

        // Обновляем состояние системы
        auto const &ss = subsystems_[idx];
        node::set_port_value(ss.port, { *ss.locked });

        eng::log::info("{}: {}", ss.name, *ss.locked);
    });
}

