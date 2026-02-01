#pragma once

#include <string>
#include <filesystem>

namespace http_server
{

    void start(std::filesystem::path);

    void add_slave(std::string const &, std::string &&);

    void ws_notify(std::string const &, std::string const &, std::string const &);

}
