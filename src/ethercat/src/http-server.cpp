// #include "ethercat.hpp"

#include <eng/http/server.hpp>
#include <eng/http/request.hpp>
#include <eng/http/response.hpp>
// #include <coeng/buffer.hpp>
// #include <coeng/http/ws-request.hpp>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>

static std::unordered_map<std::string, std::string> slaves_;
static std::map<std::pair<std::string, std::string>, std::string> data_;

static void initial_page(eng::http::request const &request, eng::http::response &response)
{
    std::string initial_json;

    // Формируем json представление конфигурации системы
    initial_json += "{";
    std::ranges::for_each(slaves_, [&initial_json](auto const &pair)
    {
        std::filesystem::path path("www/drivers");
        std::ifstream file(path / pair.second);
        if (!file.is_open())
        {
            std::cout << "ERROR: Cannot open file: " << (path / pair.second) << std::endl;
            return;
        }

        std::string json(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        initial_json += initial_json.back() != '{' ? "," : "";
        initial_json += std::format("\"{}\": {}", pair.first, json);
    });
    initial_json += "}";

    response.set_content(std::move(initial_json), "application/json");
    response.status = eng::http::status_code::OK_200;
}

static void bit_set(eng::http::request const &request, eng::http::response &response)
{
    auto guid = request.get_param("guid");
    auto key = request.get_param("key");
    auto value = request.get_param("value");

    std::cout << "bit-set: guid = " << *guid << ", key = " << *key << ", value = " << *value << std::endl;

    // ethercat::set_bit(*guid, *key, *value);

    response.set_content("{}", "application/json");
    response.status = eng::http::status_code::OK_200;
}

static void bit_reset(eng::http::request const &request, eng::http::response &response)
{
    auto guid = request.get_param("guid");
    auto key = request.get_param("key");
    auto value = request.get_param("value");

    std::cout << "bit-reset: guid = " << *guid << ", key = " << *key << ", value = " << *value << std::endl;

    // ethercat::reset_bit(*guid, *key, *value);

    response.set_content("{}", "application/json");
    response.status = eng::http::status_code::OK_200;
}

static void value_set(eng::http::request const &request, eng::http::response &response)
{
    auto guid = request.get_param("guid");
    auto key = request.get_param("key");
    auto value = request.get_param("value");

    std::cout << "value-set: guid = " << *guid << ", key = " << *key << ", value = " << *value << std::endl;

    // ethercat::set_value(*guid, *key, *value);

    response.set_content("{}", "application/json");
    response.status = eng::http::status_code::OK_200;
}

// static std::string create_ws_notify(std::string const &guid, std::string const &key, std::string const &value)
// {
//     std::string json("[\"");
//     json += guid;
//     json += "\",\"";
//     json += key;
//     json += "\",\"";
//     json += value;
//     json += "\"]";
//     return json;
// }

// static coeng::promise_void new_ws_connected(coeng::tcp::tcp &stream)
// {
//     if (data_.empty()) co_return;
//
//     std::string msg = "[";
//     for (auto const &ditem : data_)
//     {
//         if (msg.size() > 1) msg += ", ";
//         msg += create_ws_notify(ditem.first.first, ditem.first.second, ditem.second);
//     }
//     msg += "]";
//
//     coeng::buffer buf(msg.length());
//
//     std::uint8_t const *data = reinterpret_cast<std::uint8_t const *>(msg.data());
//     std::uint8_t *dst = reinterpret_cast<std::uint8_t *>(buf.data());
//     std::uint64_t const len = coeng::http::ws::server_wrap(dst, data, msg.length(), 1, 1, 1, 0);
//
//     co_await stream.write({ std::as_bytes(std::span{ buf.data(), len }) });
// }

namespace http_server
{

    eng::http::server server;

    void start(std::filesystem::path root)
    {
        server.set_home_directory(root);
        server.run("0.0.0.0", 8090);

        server.get("/initial-page", &initial_page);

        server.get("/bit-set", &bit_set);
        server.get("/bit-reset", &bit_reset);

        server.get("/value-set", &value_set);

        // server.on_ws_connected(&new_ws_connected);
    }

    void add_slave(std::string const &name, std::string &&file_name)
    {
        slaves_.emplace(name, std::move(file_name));
    }

    void ws_notify(std::string const &guid, std::string const &key, std::string const &value)
    {
        // data_[std::make_pair(guid, key)] = value;
        //
        // std::string msg = "[";
        // msg += create_ws_notify(guid, key, value);
        // msg += "]";
        //
        // http_.send_ws_text(msg);
    }

}
