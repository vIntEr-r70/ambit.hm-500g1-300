#include "units-interaction.hpp"
#include "modbus-unit.hpp"

#include <eng/modbus/regs-map.hpp>

#include <eng/modbus/operation/read_holding_registers.hpp>
#include <eng/modbus/operation/write_single_register.hpp>
#include <eng/modbus/operation/write_multiple_registers.hpp>
#include <eng/modbus/tcp/frame.hpp>
#include <eng/modbus/pdu.hpp>
#include <eng/modbus/tcp/receiver.hpp>
#include <eng/utils.hpp>
#include <eng/tcp/client.hpp>
#include <eng/result.hpp>
#include <eng/uv/writer.hpp>
#include <eng/uv/reader.hpp>
#include <eng/timer.hpp>
#include <eng/log.hpp>

#include <list>
#include <algorithm>

namespace global
{

    struct client_t;

    typedef void(*response_handler_t)(client_t *);

    struct operation_t
    {
        response_handler_t handler;
        modbus::tcp::frame frame;
        bool immediate;
    };

    struct client_t
    {
        modbus_unit *unit;

        uv_stream_t *stream;

        eng::modbus::regs_map regs_map;
        modbus::tcp::receiver receiver{ true };

        std::list<operation_t> ops_queue;
        std::list<operation_t> ops_immediate_queue;
        std::list<operation_t> ops_cache;

        // Какая операция выполняется в данный момент
        bool immediate;
    };
    static std::vector<client_t*> clients;

    static void close_unit_connection(client_t *);

    static void holding_registers_request(client_t *);
    static void wait_unit_response(client_t *);

    static void read_holding_response(client_t *);
    static void write_multiple_response(client_t *);
    static void write_single_response(client_t *);

    static void do_next_operation(client_t *);

    static operation_t& prepare_operation(client_t *client, response_handler_t handler, bool immediate)
    {
        auto &ops_cache = client->ops_cache;
        auto &ops_queue = client->ops_queue;
        auto &ops_immediate_queue = client->ops_immediate_queue;

        // Если в списке свободных пусто, создаем новую
        if (ops_cache.empty())
            ops_cache.emplace(ops_cache.end());

        // Если это первая операция в очереди, инициируем процесс выполнения
        if (ops_queue.empty() && ops_immediate_queue.empty())
            eng::timer::once([client] { do_next_operation(client); } );

        // Если задача срочная, помещаем ее в конец приоритетных задач
        auto &queue = immediate ? ops_immediate_queue : ops_queue;
        queue.splice(queue.end(), ops_cache, ops_cache.begin());

        // Возвращаем ссылку на новую операцию
        operation_t &op = immediate ? *ops_immediate_queue.rbegin() : *ops_queue.rbegin();
        op.handler = handler;
        op.immediate = immediate;

        return op;
    }

    static void do_next_operation(client_t *client)
    {
        // Запоминаем, из какой очеререди мы взяли операцию
        client->immediate = !client->ops_immediate_queue.empty();

        // Получаем очередь откуда будем брать очередную
        // задачу учитывая приоритет очередей
        auto &queue = client->immediate ?
            client->ops_immediate_queue : client->ops_queue;

        // Операций больше нету, прекращаем выполнение
        if (queue.empty())
            return;

        operation_t &op = queue.front();

        auto request{ std::as_bytes(std::span{ op.frame.data(), op.frame.size() }) };

        // Отправляем его серверу
        eng::uv::write(client->stream, { request },
            [client](eng::result result)
            {
                if (result == eng::result::success)
                {
                    client->receiver.clear();
                    wait_unit_response(client);
                }
                else
                {
                    close_unit_connection(client);
                }
            });
    }

    static void connect(client_t *client)
    {
        std::string host(client->unit->host());
        std::uint16_t port(client->unit->port());

        eng::log::info("connect: IP: {}, PORT: {}", host, port);

        // Инициируем установку соединения с устройством
        eng::tcp::connect(host, port, [client](uv_stream_t *stream)
        {
            // Если не удалось установить соединение, через 1 минуту повторяем попытку
            if (stream == nullptr)
            {
                eng::timer::set_single_ms(1000, [=]
                {
                    connect(client);
                });
                return;
            }

            std::string host(client->unit->host());
            std::uint16_t port(client->unit->port());
            eng::log::info("connection established: IP: {}, PORT: {}", host, port);

            client->stream = stream;
            client->unit->connected();

            client->unit->for_each_read_holding(
                [&client](std::size_t ikey, std::uint16_t address, std::size_t number_of, std::uint32_t msecs)
                {
                    client->regs_map.insert(ikey, address, number_of, msecs);
                });

            if (client->regs_map.initialize())
                holding_registers_request(client);
        });
    }

    static void close_unit_connection(client_t *client)
    {
        std::string host(client->unit->host());
        std::uint16_t port(client->unit->port());
        eng::log::info("close_unit_connection: IP: {}, PORT: {}", host, port);

        client->unit->disconnected();
    }

    // Функция инициирует запрос на чтение значений очередной группы регистров
    static void holding_registers_request(client_t *client)
    {
        // Получаем текущую по времени группу регистров для чтения
        auto const &group = client->regs_map.get_readable_group();

        // Создаем обычную операцию для выполнения операции чтения
        auto &op = prepare_operation(client, &read_holding_response, false);
        op.frame.set_unit_id(client->unit->id());

        // Формируем запрос
        modbus::request::read_holding_registers<modbus::tcp::frame>
                { op.frame, group.address, group.number_of };
    }

    static void wait_unit_response(client_t *client)
    {
        eng::uv::read(client->stream, [=](std::span<std::byte const> readed)
        {
            if (readed.empty())
            {
                close_unit_connection(client);
                return;
            }

            if (!client->receiver.is_completed(readed))
            {
                wait_unit_response(client);
                return;
            }

            // Операция выполнена

            // Получаем очередь откуда была взята операция
            auto &queue = client->immediate ?
                client->ops_immediate_queue : client->ops_queue;

            // Вызываем обработчик ответа
            operation_t &op = queue.front();
            (*op.handler)(client);

            // Возвращаем операцию в список свободных
            client->ops_cache.splice(client->ops_cache.end(), queue, queue.begin());

            // Выполняем следующую операцию
            do_next_operation(client);
        });
    }

    static void read_holding_response(client_t *client)
    {
        modbus::tcp::frame &frame = client->receiver.frame();
        modbus::response::read_holding_registers<modbus::tcp::frame>
            response{ modbus::pdu<modbus::tcp::frame>(frame) };

        client->regs_map.for_all_records_in_readable_group(response.payload(),
                [=](std::size_t ikey, std::span<std::uint8_t const> regs)
                {
                    client->unit->read_holding_done(ikey, regs);
                });

        auto mseconds = client->regs_map.prepare_next_readable_group();
        eng::timer::once(std::chrono::milliseconds(mseconds), [=]
        {
            holding_registers_request(client);
        });
    }

    static void write_single_response(client_t *client)
    {
        modbus::tcp::frame &frame = client->receiver.frame();
        modbus::response::write_single_register<modbus::tcp::frame>
            response{ modbus::pdu<modbus::tcp::frame>(frame) };
        client->unit->write_done();
    }

    static void write_multiple_response(client_t *client)
    {
        modbus::tcp::frame &frame = client->receiver.frame();
        modbus::response::write_multiple_registers<modbus::tcp::frame>
            response{ modbus::pdu<modbus::tcp::frame>(frame) };
        client->unit->write_done();
    }

}

namespace units_interaction
{

    std::size_t add_new_unit(modbus_unit *node)
    {
        std::size_t idx = global::clients.size();
        global::clients.push_back(new global::client_t{ node });
        return idx;
    }

    void start()
    {
        // Запускаем цикл работы для каждого устройства
        std::ranges::for_each(global::clients, [](auto client)
        {
            connect(client);
        });
    }

    void write_single(std::size_t idx, std::uint16_t address, std::uint16_t value)
    {
        // Получаем интересующее нас устройство
        auto client = global::clients[idx];

        if (!client->stream)
        {
            eng::log::info("[{}]: Отсутствует подключение к устройству", idx);
            return;
        }

        // Встраиваем в конец приоритетной очереди команду на запись
        auto &op = global::prepare_operation(client, &global::write_single_response, true);
        op.frame.set_unit_id(client->unit->id());

        // Формируем запрос
        modbus::request::write_single_register
            <modbus::tcp::frame>{ op.frame, address, value };
    }

    void write_multiple(std::size_t idx, std::uint16_t address, std::span<std::uint16_t const> values)
    {
        // Получаем интересующее нас устройство
        auto client = global::clients[idx];

        if (!client->stream)
        {
            eng::log::info("[{}]: Отсутствует подключение к устройству", idx);
            return;
        }

        // Встраиваем в конец приоритетной очереди команду на запись
        auto &op = global::prepare_operation(client, &global::write_multiple_response, true);
        op.frame.set_unit_id(client->unit->id());

        // Формируем запрос
        modbus::request::write_multiple_registers
            <modbus::tcp::frame> request{ op.frame, address };

        std::ranges::for_each(values, [&request](std::uint16_t value) {
            request.push(value);
        });
    }

}
