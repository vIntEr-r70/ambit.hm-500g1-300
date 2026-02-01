#include "modbus-rtu-master.hpp"
#include "modbus-unit.hpp"

#include <eng/rtu/serial-port.hpp>
#include <eng/timer.hpp>
#include <eng/log.hpp>

#include <eng/modbus/regs-map.hpp>
#include <eng/modbus/rtu/frame.hpp>
#include <eng/modbus/rtu/receiver.hpp>
#include <eng/modbus/pdu.hpp>
#include <eng/modbus/operation/read_holding_registers.hpp>
#include <eng/modbus/operation/write_single_register.hpp>
#include <eng/modbus/operation/write_multiple_registers.hpp>

#include <list>
#include <algorithm>
#include <chrono>

// #define LOG

namespace master
{

    struct client_t
    {
        modbus_unit *unit;
        eng::modbus::regs_map regs_map;
    };
    static std::list<client_t> clients;

}

namespace port
{

    static eng::rtu::serial_port port;
    static modbus::rtu::receiver receiver{ false };
    static std::size_t writed_bytes = 0;

    static void do_reading();
    static void do_writing();
    static void do_next_operation();

    typedef void(*response_handler_t)(master::client_t *);

    struct operation_t
    {
        master::client_t *client;
        modbus::rtu::frame frame;
        response_handler_t handler;

        std::size_t failed_count;
    };

    static std::list<operation_t> requests;
    static std::list<operation_t> requests_cache;

    static eng::timer::id_t timeout_ctl_timer_;
    static eng::timer::id_t operaton_delay_timer_;

    static void continue_reading()
    {
        operaton_delay_timer_ = eng::timer::once(
            std::chrono::milliseconds(10),
            [] { do_reading(); });
    }

    static void continue_writing()
    {
        operaton_delay_timer_ = eng::timer::once(
            std::chrono::milliseconds(10),
            [] { do_writing(); });
    }

    // Берем свободную или создаем новую ячейку для описания задания
    // Если это первая операция, запускаем таймер выполнения
    static operation_t& prepare_operation(master::client_t *client, response_handler_t handler)
    {
        if (requests_cache.empty())
            requests_cache.emplace(requests_cache.end());

        // Если это первая операция в очереди, инициируем процесс выполнения
        if (requests.empty())
            eng::timer::once([] { do_next_operation(); } );

        // Помещаем новую операцию в конец очереди
        requests.splice(requests.end(), requests_cache, requests_cache.begin());

        // Возвращаем ссылку на новую операцию
        auto &op = *requests.rbegin();

        op.client = client;
        op.handler = handler;
        op.failed_count = 0;

        return op;
    }

    static void operation_success_done()
    {
        // Останавливаем таймер контроля
        eng::timer::kill_timer(timeout_ctl_timer_);

        operation_t &op = requests.front();
        (*op.handler)(op.client);

        // Возвращаем операцию в список свободных
        requests_cache.splice(requests_cache.begin(), requests, requests.begin());

        // Инициируем следующую операцию
        do_next_operation();
    }

    static void replace_all_client_operations(master::client_t *client)
    {
        // Если у данного клиента есть и другие операции в списке, удаляем их
        auto it = requests.begin();
        while (it != requests.end())
        {
            if (it->client != client)
            {
                it = std::next(it);
                continue;
            }

            auto need_replace = it;
            it = std::next(it);

            requests_cache.splice(requests_cache.begin(), requests, need_replace);
        }
    }

    // Если в процессе выполнения операции произошла ошибка:
    // - не удалось записать запрос или прочитать корректный ответ
    // - контрольная сумма ответа не сошлась
    // Мы помещаем данную операцию в конец очереди для следующей попытки
    static void operation_failed_done()
    {
        // Останавливаем таймер контроля
        eng::timer::kill_timer(timeout_ctl_timer_);

        operation_t &op = requests.front();
        op.failed_count += 1;

        // Если нам так и не удалось получить ответ от устройства
        if (op.failed_count >= 3)
        {
            op.client->unit->offline();
            replace_all_client_operations(op.client);
        }

        // Чистим порт перед очередным запросом
        port.purge(eng::rtu::serial_port::purge_type::all);

        // Инициируем следующую операцию
        do_next_operation();
    }

    // В процессе выполнения операции на порту произошла системная ошибка
    // Прекращаем все работы на устройстве и уведомляем пользователей
    static void terminate_port_operations()
    {
        // Останавливаем таймер контроля
        eng::timer::kill_timer(timeout_ctl_timer_);
    }

    // Читаем ответ от устройства
    static void do_reading()
    {
        try
        {
            // Читаем данные
            for (;;)
            {
                auto buf = receiver.buffer();
                std::size_t received = port.read(buf.first, buf.second);

                // Мы прочитали все доступные в порту данные, выходим из цикла
                if (received == 0)
                {
                    continue_reading();
                    return;
                }

                int result;
                if (receiver.is_completed(received, result))
                    break;
            }

#ifdef LOG
            eng::log::info("[{}] modbus-rtu-master::do_reading: {}", port.name(),
                    eng::utils::to_hex(receiver.frame().data(), receiver.frame().size()));
#endif

            if (!receiver.frame().is_valid())
            {
                eng::log::error("[{}] modbus-rtu-master::do_reading: {}", port.name(), "frame is invalid");
                operation_failed_done();
            }
            else
            {
                operation_success_done();
            }
        }
        catch (std::exception const &e)
        {
            eng::log::error("[{}] modbus-rtu-master::do_reading: {}", port.name(), e.what());
            terminate_port_operations();
            return;
        }
    }

    // Передаем запрос устройству
    static void do_writing()
    {
        operation_t &op = requests.front();

        try
        {
            // Открываем порт если он еще не открыт
            if (!port.is_open()) port.open();

#ifdef LOG
            eng::log::info("[{}] modbus-rtu-master::do_writing: {}", port.name(),
                    eng::utils::to_hex(op.frame.data() + writed_bytes, op.frame.size() - writed_bytes));
#endif
            std::size_t writed = port.write(
                    op.frame.data() + writed_bytes,
                    op.frame.size() - writed_bytes);

            // Ничего не удалось записать, подождем
            if (writed == 0)
            {
                continue_writing();
                return;
            }

            // Убираем из буффера те данные, которые были записаны
            writed_bytes += writed;
        }
        catch (std::exception const &e)
        {
            eng::log::error("[{}] modbus-rtu-master::do_writing: {}", port.name(), e.what());
            terminate_port_operations();
            return;
        }

        // Если данные еще остались,
        // продолжаем ждать готовности порта на запись
        if (op.frame.size() != writed_bytes)
        {
            continue_writing();
            return;
        }

        // Операция выполнена, стартуем новый таймер
        // ожидания выполнения операции чтения
        eng::timer::kill_timer(timeout_ctl_timer_);

        timeout_ctl_timer_ = eng::timer::once(
                std::chrono::milliseconds(100),
                []
                {
                    eng::log::error("[{}] modbus-rtu-master: read operation timeout", port.name());
                    // Время ожидания на чтение команды вышло
                    // Останавливаем выполнение операции
                    eng::timer::kill_timer(operaton_delay_timer_);

                    operation_failed_done();
                });

#ifdef LOG
        eng::log::info("[{}] modbus-rtu-master::do_writing: start reading", port.name());
#endif

        // Готовим приемник
        receiver.clear();

        continue_reading();
    }

    // Инициализация выполнения следующей операции
    static void do_next_operation()
    {
        // Операций больше нету, прекращаем выполнение
        if (requests.empty())
            return;

#ifdef LOG
        eng::log::info("modbus-rtu-master: do_next_operation");
#endif

        // Стартуем таймер который должен будет прервать
        // операцию по превышению времени выполнения
        timeout_ctl_timer_ = eng::timer::once(
                std::chrono::milliseconds(100),
                []
                {
                    eng::log::error("modbus-rtu-master: write operation timeout");
                    // Время ожидания на отправку команды вышло
                    // Останавливаем выполнение операции
                    eng::timer::kill_timer(operaton_delay_timer_);

                    operation_failed_done();
                });

        // Сбрасываем счетчик записанных байтов
        writed_bytes = 0;

        continue_writing();
    }
}

namespace master
{

    static void read_holding_response(client_t *);
    static void write_multiple_response(client_t *);

    // Функция инициирует запрос на чтение значений очередной группы регистров
    static void holding_registers_request(client_t *client)
    {
        // Резервируем место для задачи в очереди порта
        auto &op = port::prepare_operation(client, &read_holding_response);
        op.frame.set_unit_id(client->unit->id());

        // Получаем текущую по времени группу регистров для чтения
        auto const &group = client->regs_map.get_readable_group();

        // Формируем запрос
        modbus::request::read_holding_registers<modbus::rtu::frame>
                { op.frame, group.address, group.number_of };
        op.frame.update_checksum();
    }

    static void read_holding_response(client_t *client)
    {
        modbus::rtu::frame &frame = port::receiver.frame();
        modbus::response::read_holding_registers<modbus::rtu::frame>
            response{ modbus::pdu<modbus::rtu::frame>(frame) };

        client->regs_map.for_all_records_in_readable_group(response.payload(),
                [=](std::size_t ikey, std::span<std::uint8_t const> regs)
                {
                    client->unit->read_holding_done(ikey, regs);
                });

        auto mseconds = client->regs_map.prepare_next_readable_group();
        eng::timer::once(std::chrono::milliseconds(mseconds),
        [=]
        {
            holding_registers_request(client);
        });
    }

    static void write_multiple_response(client_t *client)
    {
        modbus::rtu::frame &frame = port::receiver.frame();
        modbus::response::write_multiple_registers<modbus::rtu::frame>
            response{ modbus::pdu<modbus::rtu::frame>(frame) };

        client->unit->write_done(0);
    }

    void write_single_response(client_t *client)
    {
        modbus::rtu::frame &frame = port::receiver.frame();
        modbus::response::write_single_register<modbus::rtu::frame>
            response{ modbus::pdu<modbus::rtu::frame>(frame) };

        client->unit->write_done(0);
    }

    client_t *find_client_by_unit(modbus_unit const *unit)
    {
        auto it = std::ranges::find_if(master::clients, [=](auto const &client) {
            return client.unit == unit;
        });
        return (it == master::clients.end()) ? nullptr : &*it;
    }

}

namespace modbus_rtu_master
{

    void init(std::string_view port_name)
    {
        port::port.set_name(port_name);
        port::port.set_baud_rate(57600);
        port::port.set_char_size(eng::rtu::serial_port::char_size::cs8);
        port::port.set_stop_bit(eng::rtu::serial_port::stop_bit::one);
        port::port.set_parity(eng::rtu::serial_port::parity::no);

        // Запускаем цикл работы для каждого устройства
        std::ranges::for_each(master::clients, [](auto &client)
        {
            client.unit->for_each_read_holding(
                [&client](std::size_t ikey, std::uint16_t address, std::size_t number_of, std::uint32_t msecs)
                {
                    client.regs_map.insert(ikey, address, number_of, msecs);
                });

            if (!client.regs_map.initialize())
                return;

            master::holding_registers_request(&client);
        });
    }

    void add_new_unit(modbus_unit *unit)
    {
        master::clients.emplace(master::clients.end(), unit);
    }

    void restart(modbus_unit *unit)
    {
        master::client_t *client = master::find_client_by_unit(unit);
        if (client == nullptr) return;

        if (!client->regs_map.initialize())
            return;

        master::holding_registers_request(client);
    }

    void destroy()
    {
        // global::port.close();
    }

    void write_multiple(modbus_unit const *unit, std::uint16_t address, std::span<std::uint16_t const> data)
    {
        master::client_t *client = master::find_client_by_unit(unit);
        if (client == nullptr) return;

        auto &op = port::prepare_operation(client, &master::write_multiple_response);
        op.frame.set_unit_id(unit->id());

        modbus::request::write_multiple_registers
                <modbus::rtu::frame> request{ op.frame, address };

        std::ranges::for_each(data, [&request](std::uint16_t value) {
            request.push(value);
        });

        op.frame.update_checksum();
    }

    void write_single(modbus_unit const *unit, std::uint16_t address, std::uint16_t value)
    {
        master::client_t *client = master::find_client_by_unit(unit);
        if (client == nullptr) return;

        auto &op = port::prepare_operation(client, &master::write_single_response);
        op.frame.set_unit_id(unit->id());

        modbus::request::write_single_register
            <modbus::rtu::frame>{ op.frame, address, value };

        op.frame.update_checksum();
    }

}

