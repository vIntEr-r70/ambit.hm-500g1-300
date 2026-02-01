#include "intervals.hpp"

#include <eng/json.hpp>
#include <eng/log.hpp>
#include <eng/sibus/client.hpp>

#include <algorithm>
#include <bitset>

intervals::intervals(std::filesystem::path const &path)
    : eng::sibus::node("intervals")
{
    if (!load_config(path / "intervals.json"))
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

            configure_relationship(idx, cfg.get<std::uint32_t>("mask"));
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

void intervals::update_locker(std::size_t idx, double value)
{
    auto &ll = lockers_[idx];

    // Запоминаем последнее значение
    ll.value = value;

    if (std::isnan(value))
    {
        update_locker_status(idx, lstatus::critical);
        return;
    }

    auto result = lstatus::normal;

    if (!is_in_range(value, ll.range[1], ll.range[2]))
        result = lstatus::warning;

    if (!is_in_range(value, ll.range[0], ll.range[3]))
        result = lstatus::critical;

    update_locker_status(idx, result);
}

void intervals::update_locker_status(std::size_t idx, lstatus value)
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
        update_subsystem(idx);
    });
}

void intervals::update_subsystem(std::size_t idx)
{
    auto &ss = subsystems_[idx];

    // Ищем хоть одно критическое состояние
    auto critical = std::ranges::find_if(ss.lockers,
        [this](std::size_t idx) {
            return lockers_[idx].status == lstatus::critical;
        });

    bool locked = (critical != ss.lockers.end());

    // Состояние не изменилось
    if (ss.locked && (*ss.locked == locked))
        return;
    ss.locked = locked;

    // Обновляем состояние системы
    node::set_port_value(ss.port, { *ss.locked });
    eng::log::info("{}: {}", ss.name, *ss.locked);
}

// Необходим статический конфигурационный файл,
// содержащий список контролируемых параметров
// и список подсистем, которые можно блокировать
// {
//      "lockers": {
//          "FC1": "FC1",
//          "DT1": "DT1",
//      },
//      "subsystems": {
//          "fc": "fc"
//          "moving": "moving"
//          "sprayer": "sprayer"
//      }
// }

bool intervals::load_config(std::filesystem::path const &fname)
{
    try
    {
        eng::json::object cfg(fname);

        cfg.get_object("lockers").for_each([this](std::string_view sv_key, eng::json::value)
        {
            lockers_.emplace_back(std::string(sv_key));
        });

        cfg.get_object("subsystems").for_each([this](std::string_view sv_key, eng::json::value)
        {
            subsystems_.emplace_back(std::string(sv_key));
        });

        return true;
    }
    catch(std::exception const &e)
    {
        eng::log::error("load_config: {}", e.what());
    }

    return false;
}

void intervals::configure_relationship(std::size_t idx, std::uint32_t mask)
{
    auto &locker = lockers_[idx];
    locker.subsystems.clear();

    // Маска контроллируемых данным параметром подсистем
    std::bitset<sizeof(mask) * CHAR_BIT> bits(mask);
    for (std::size_t i = 0; i < subsystems_.size(); ++i)
    {
        auto &subsystem = subsystems_[i];
        auto it = std::ranges::find(subsystem.lockers, idx);

        if (!bits.test(i))
        {
            if (it != subsystem.lockers.end())
                subsystem.lockers.erase(it);
            continue;
        }
        locker.subsystems.push_back(i);

        if (it == subsystem.lockers.end())
            subsystem.lockers.push_back(idx);
    }

    if (std::isnan(!locker.value))
        update_locker(idx, locker.value);

    // std::stringstream ss;
    // std::ranges::for_each(locker.subsystems, [&ss, this](std::size_t idx)
    // {
    //     if (!ss.view().empty()) ss << ", ";
    //     ss << subsystems_[idx].name;
    // });
    // eng::log::info("{}: [{}]", locker.name, ss.view());
    //
    // std::ranges::for_each(subsystems_, [this](auto const &ssys)
    // {
    //     std::stringstream ss;
    //     std::ranges::for_each(ssys.lockers, [&ss, &ssys, this](std::size_t idx)
    //     {
    //         if (!ss.view().empty()) ss << ", ";
    //         ss << lockers_[idx].name;
    //     });
    //     eng::log::info("{}: [{}]", ssys.name, ss.view());
    // });
    // eng::log::info("");
}

