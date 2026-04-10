#include "modbus-tcp-server.hpp"
#include "base-device.hpp"

#include <eng/tcp/listener.hpp>
#include <eng/uv/writer.hpp>
#include <eng/uv/reader.hpp>
#include <eng/modbus/tcp/receiver.hpp>
#include <eng/modbus/pdu.hpp>
#include <eng/utils.hpp>
#include <eng/modbus/tcp/frame.hpp>
#include <eng/modbus/error.hpp>
#include <eng/modbus/pdu.hpp>
#include <eng/pipe/listener.hpp>

#include <eng/modbus/operation/read_holding_registers.hpp>
#include <eng/modbus/operation/write_multiple_registers.hpp>
#include <eng/modbus/operation/write_single_register.hpp>

#include <filesystem>
#include <format>
#include <list>

namespace context
{

    struct peer_t
    {
        devices::modbus_device *device;
        uv_stream_t *stream;

        eng::modbus::tcp::receiver receiver{ true };
        eng::modbus::tcp::frame response_frame;

        std::list<peer_t>::const_iterator iterator;
    };

    static std::list<peer_t> peers;
    static std::list<peer_t> cache;

    static peer_t& get_awailable_peer()
    {
        if (cache.empty())
        {
            cache.emplace(cache.begin());
            cache.front().iterator = cache.begin();
        }

        peers.splice(peers.end(), cache, cache.begin());

        return peers.back();
    }

    static void free_peer(peer_t &peer)
    {
        cache.splice(cache.end(), peers, peer.iterator);
    }

    static void read_requests(peer_t &);

    static void close_connection(peer_t &peer)
    {
        free_peer(peer);
    }

    static void write_response(peer_t &peer)
    {
        auto data = std::span{ peer.response_frame.data(), peer.response_frame.size() };

        eng::uv::write(peer.stream, { std::as_bytes(data) },
                [&peer](eng::result)
                {
                    peer.receiver.clear();
                    read_requests(peer);
                });
    }

    static void send_exception_response(peer_t &peer, eng::modbus::error_code::type error)
    {
        using response = eng::modbus::response::error<eng::modbus::tcp::frame>;

        auto &frame = peer.receiver.frame();
        eng::modbus::pdu<eng::modbus::tcp::frame> const pdu(frame);
        response resp{ peer.response_frame, static_cast<int>(pdu.op_code()), error };

        write_response(peer);
    }

    static void send_read_holding_registers_response(peer_t &peer)
    {
        using request = eng::modbus::request::read_holding_registers<eng::modbus::tcp::frame>;
        using response = eng::modbus::response::read_holding_registers<eng::modbus::tcp::frame>;

        auto &frame = peer.receiver.frame();
        eng::modbus::pdu<eng::modbus::tcp::frame> const pdu(frame);

        request const read_op{pdu};

        response resp{ peer.response_frame };
        for (std::size_t i = 0; i < read_op.number_of(); ++i)
            resp.push(peer.device->get(read_op.address() + i));

        write_response(peer);
    }

    static void send_write_single_register_response(peer_t &peer)
    {
        using request = eng::modbus::request::write_single_register<eng::modbus::tcp::frame>;
        using response = eng::modbus::response::write_single_register<eng::modbus::tcp::frame>;

        auto &frame = peer.receiver.frame();
        eng::modbus::pdu<eng::modbus::tcp::frame> const pdu(frame);

        request const write_op{ pdu };
        peer.device->set(write_op.address(), write_op.value());

        response resp{ peer.response_frame };

        write_response(peer);
    }

    static void send_write_multiple_registers_response(peer_t &peer)
    {
        using request = eng::modbus::request::write_multiple_registers<eng::modbus::tcp::frame>;
        using response = eng::modbus::response::write_multiple_registers<eng::modbus::tcp::frame>;

        auto &frame = peer.receiver.frame();
        eng::modbus::pdu<eng::modbus::tcp::frame> const pdu(frame);

        request const write_op{ pdu };
        for (std::size_t i = 0; i < write_op.number_of(); ++i)
            peer.device->set(write_op.address() + i, write_op.get(i));

        response resp{ peer.response_frame };

        write_response(peer);
    }

    static void read_requests(peer_t &peer)
    {
        eng::uv::read(peer.stream, [&peer, stream = peer.stream](std::span<std::byte const> readed)
        {
            if (readed.empty())
            {
                close_connection(peer);
                return;
            }

            if (!peer.receiver.is_completed(readed))
            {
                read_requests(peer);
                return;
            }

            auto &frame = peer.receiver.frame();
            eng::modbus::pdu<eng::modbus::tcp::frame> const pdu(frame);

            switch(pdu.op_code())
            {
            case eng::modbus::operation::read_coils:
                send_exception_response(peer, eng::modbus::error_code::type::illegal_operation);
                break;
            case eng::modbus::operation::read_discrete_inputs:
                send_exception_response(peer, eng::modbus::error_code::type::illegal_operation);
                break;
            case eng::modbus::operation::read_holding_registers:
                send_read_holding_registers_response(peer);
                break;
            case eng::modbus::operation::read_input_registers:
                send_exception_response(peer, eng::modbus::error_code::type::illegal_operation);
                break;
            case eng::modbus::operation::write_single_coil:
                send_exception_response(peer, eng::modbus::error_code::type::illegal_operation);
                break;
            case eng::modbus::operation::write_single_register:
                send_write_single_register_response(peer);
                break;
            case eng::modbus::operation::write_multiple_coils:
                send_exception_response(peer, eng::modbus::error_code::type::illegal_operation);
                break;
            case eng::modbus::operation::write_multiple_registers:
                send_write_multiple_registers_response(peer);
                break;
            case eng::modbus::operation::read_write_multiple_registers:
                send_exception_response(peer, eng::modbus::error_code::type::illegal_operation);
                break;
            default:
                send_exception_response(peer, eng::modbus::error_code::type::illegal_operation);
                break;
            }
        });
    }

    static void add_new_client(uv_stream_t *stream, devices::modbus_device *device)
    {
        auto &peer = get_awailable_peer();

        peer.stream = stream;
        peer.device = device;
        peer.receiver.clear();

        read_requests(peer);
    }

}

namespace mts
{

    eng::pipe::listener_id_t add_listener(devices::modbus_device &device)
    {
#ifdef _WIN32
        std::string ipc = std::format("\\\\.\\pipe\\{}", device.name());
#else
        std::string ipc = std::format("/tmp/{}", device.name());
#endif

        if (std::filesystem::exists(ipc))
            std::filesystem::remove(ipc);

        return eng::pipe::start_listen(ipc, [ptr=&device](uv_stream_t *stream)
        {
            context::add_new_client(stream, ptr);
        });
    }

    void remove_listener(eng::pipe::listener_id_t id)
    {
        eng::pipe::stop_listen(id);
    }

}
