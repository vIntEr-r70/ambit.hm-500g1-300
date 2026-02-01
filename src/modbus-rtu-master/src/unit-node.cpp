#include "unit-node.hpp"
#include "units-interaction.hpp"

#include <numeric>
#include <algorithm>

namespace
{

    static std::vector<unit_node*> units;

    static bool modbus_data_reordering(std::uint8_t *dst, std::uint8_t const *src, std::size_t size, unit_conf_loader::order_t &order)
    {
        static std::vector<std::uint8_t> memory;
        memory.resize(size);

        // Убеждаемся что мы работаем в рамках преобразуемого значения
        if (size <= order.size())
        {
            for (std::size_t i = 0; i < size; ++i)
                memory[i] = src[order[i]];
        }
        else
        {
            // Если это не так, например строка, заменяем попарно байты в регистре
            // выполняя соглашение Modbus о порядке байтов в регистре
            for (std::size_t i = 0; i < (size >> 1); ++i)
            {
                memory[i + 0] = src[i + 1];
                memory[i + 1] = src[i + 0];
            }
        }

        if (std::memcmp(dst, memory.data(), size))
        {
            std::memcpy(dst, memory.data(), size);
            return true;
        }

        return false;
    }

}

unit_node::unit_node(std::filesystem::path const &fname)
    : eng::sibus::node(fname.stem().native())
{
    read_unit_conf_file(name(), fname);

    // Регистрируем выходные порты для выдачи значений
    std::ranges::for_each(records_, [this](unit_conf_record_t &record)
    {
        record.port_id = add_output_port(record.name);
    });
    
    units_interaction::add_new_unit(units.size());
    units.push_back(this);
    
    for (std::size_t ikey = 0; ikey < records_.size(); ++ikey)
    {
        unit_conf_record_t const &r = records_[ikey];
        units_interaction::add_last_unit_record(ikey, r.number_of, r.record);
    }

    // Обработчик данных на выходных контактах
    // Просто передаем шине значения
    units_interaction::on_modbus_unit_data_changed(
        [=](std::size_t unit_idx, std::size_t conf_record_idx, std::span<std::uint8_t const> payload)
        {
            auto unit = units[unit_idx];
            unit->on_modbus_unit_data_changed(conf_record_idx, payload);
        });
}

void unit_node::on_modbus_unit_data_changed(std::size_t conf_record_idx, std::span<std::uint8_t const> payload)
{
    // Выясняем идентификатор узла и выходного контакта
    auto &record = records_[conf_record_idx];
    std::uint8_t const *readed_bytes = payload.data();
    std::uint8_t *dst_bytes = record.memory.data();

    for (std::size_t i = 0; i < record.record.items_count; ++i)
    {
        std::size_t bytes_count{ record.number_of * 2 };

        // Выполняем переупорядочевание байтов согласно конфигурации
        if (modbus_data_reordering(dst_bytes, readed_bytes, bytes_count, record.record.order) || record.invalidated)
        {
            eng::utils::print_hex(dst_bytes, bytes_count);

            // Преобразуем данные в значение, передаем значение шине
            eng::abc::pack args = eng::type_traits::pack(record.type_id,
                    std::as_bytes(std::span{ dst_bytes, bytes_count }));

            // Отправляем на шину
            node::set_port_value(record.port_id, std::move(args));

            record.invalidated = false;
        }

        readed_bytes += bytes_count;
        dst_bytes += bytes_count;
    }
}

bool unit_node::read_unit_conf_file(std::string_view name, std::filesystem::path const &file_name)
{
    struct cell_t {
        std::string_view text;
        std::size_t width;
    };
    cell_t cells[] = {
            { "tag", 10 },
            { "type", 10 },
            { "address", 15 },
            { "access", 10 },
            { "count", 10 }
        };

    std::size_t width = std::accumulate(cells, cells + std::size(cells), 0,
            [](std::size_t acc, auto const &cell) {
                return acc + cell.width;
            });
    width += std::size(cells) * 2;

    std::size_t half_width = (width - name.length()) / 2;

    std::println("{}:", name);
    std::println("+{}+", std::string(width - 2, '-'));
    for (auto const &cell : cells)
        std::print("|{:^{}}|", cell.text, cell.width);
    std::println("");
    std::println("+{}+", std::string(width - 2, '-'));

    bool result = unit_conf_loader::load(file_name, [&](auto record)
    {
        auto cell = std::begin(cells);
        std::print("|{:^{}}|", record.tag, (cell++)->width);
        std::print("|{:^{}}|", record.type, (cell++)->width);
        std::print("|{:^{}}|", record.address, (cell++)->width);
        auto const &acc = record.access;
        std::print("|{:^{}}|", std::format("{}{}", acc[0], acc[1]), (cell++)->width);
        std::print("|{:^{}}|", record.items_count, (cell++)->width);
        std::println("");

        auto type_id = eng::type_traits::type_id_from_name(record.type);
        std::size_t number_of = eng::type_traits::size(type_id) / 2;

        std::size_t ikey = records_.size();
        records_.emplace_back(
                std::string(record.tag),
                number_of,
                record,
                type_id
            );
        records_.back().memory.resize(number_of * record.items_count);
    });

    std::println("+{}+", std::string(width - 2, '-'));
    std::println("{}", result ? "OK" : "FAILED");

    return result;
}

