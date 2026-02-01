#include "ethercat.hpp"
#include "http-server.hpp"

#include <eng/json.hpp>

// #include <algorithm>
// #include <fstream>
// #include <iostream>

struct dict_record_t
{
    std::string_view type;  // идентификатор типа
    std::uint16_t index;    // индекс
    std::uint8_t subidx;    // подиндекс
};

template <typename T>
static T read_config_value(std::filesystem::path path, T def = {})
{
    // std::ifstream file(path);
    // if (!file.is_open())
    //     return def;
    //
    // // Читаем всё содержимое
    // std::string json(
    //     (std::istreambuf_iterator<char>(file)),
    //     std::istreambuf_iterator<char>());
    //
    // yyjson_read_flag read_flag = YYJSON_READ_ALLOW_EXT_NUMBER | YYJSON_READ_INSITU;
    //
    // yyjson_read_err err;
    // yyjson_doc *doc = yyjson_read_opts(json.data(), json.length(), read_flag, nullptr, &err);
    // if (doc == nullptr)
    // {
    //     std::cout <<
    //         std::format("ERROR: Cannot parse file[{}]: error({}): {} at position: {}",
    //                 path.native(), err.code, err.msg, err.pos) << std::endl;
    //     return def;
    // }
    //
    // yyjson_val *root = yyjson_doc_get_root(doc);
    //
    // if constexpr (std::is_integral<T>())
    // {
    //     if constexpr (std::is_unsigned<T>())
    //         return static_cast<T>(yyjson_get_uint(root));
    //     else
    //         return static_cast<T>(yyjson_get_int(root));
    // }
    // else if constexpr (std::is_floating_point<T>())
    //     return static_cast<T>(yyjson_get_num(root));

    return def;
}

static std::string read_config_string(std::filesystem::path path, std::string const &def = "")
{
    return def;

    // std::ifstream file(path);
    // if (!file.is_open())
    //     return def;
    //
    // // Читаем всё содержимое
    // std::string json(
    //     (std::istreambuf_iterator<char>(file)),
    //     std::istreambuf_iterator<char>());
    //
    // yyjson_read_flag read_flag = YYJSON_READ_ALLOW_EXT_NUMBER | YYJSON_READ_INSITU;
    //
    // yyjson_read_err err;
    // yyjson_doc *doc = yyjson_read_opts(json.data(), json.length(), read_flag, nullptr, &err);
    // if (doc == nullptr)
    // {
    //     std::cout <<
    //         std::format("ERROR: Cannot parse file[{}]: error({}): {} at position: {}",
    //                 path.native(), err.code, err.msg, err.pos) << std::endl;
    //     return def;
    // }
    //
    // yyjson_val *root = yyjson_doc_get_root(doc);
    // return std::string(yyjson_get_str(root), yyjson_get_len(root));
}

static void read_dictionary(eng::json::array array, std::function<void(dict_record_t const &)> const &func)
{
    array.for_each([&func](eng::json::value v)
    {
        eng::json::object rec(v);

        dict_record_t item{
            .type = rec.get_string("type"),
            .index = rec.get<std::uint16_t>("index"),
            .subidx = rec.get<std::uint8_t>("subidx")
        };

        func(item);
    });
}

static void read_sdo_dictionary(eng::json::object obj)
{
    if (!obj) return;

    eng::json::array ro(obj.get_array("ro"));
    if (ro) read_dictionary(ro, [](dict_record_t record) {
        ethercat::add_ro_sdo(record.index, record.subidx, record.type);
    });

    eng::json::array rw(obj.get_array("rw"));
    if (rw) read_dictionary(rw, [](dict_record_t record) {
        ethercat::add_rw_sdo(record.index, record.subidx, record.type);
    });
}

static void read_pdo_dictionary(eng::json::object obj)
{
    if (!obj) return;

    eng::json::array transmit(obj.get_array("transmit"));
    if (transmit) read_dictionary(transmit, [](dict_record_t record) {
        ethercat::add_pdo_transmit(record.index, record.subidx, record.type);
    });

    eng::json::array receive(obj.get_array("receive"));
    if (receive) read_dictionary(receive, [](dict_record_t record) {
        ethercat::add_pdo_receive(record.index, record.subidx, record.type);
    });
}

namespace unit_conf_loader
{

    void read_unit_conf_file(std::filesystem::path const &file_name)
    {
        if (!std::filesystem::is_symlink(file_name))
            return;

        std::string slave_name = file_name.filename().native();
        std::string slave_file_name = std::filesystem::read_symlink(file_name).filename().native();
        http_server::add_slave(slave_name, std::move(slave_file_name));

        eng::json::object root(file_name);

        // ethercat::add_slave(slave_name, ethercat::slave_config_t{
        //         .alias       = read_config_value<std::uint16_t>(std::format("config/{}/alias", slave_name), 0),
        //         .position    = read_config_value<std::uint16_t>(std::format("config/{}/position", slave_name), 0),
        //         .VendorID    = root.get<std::uint32_t>("VendorID"),
        //         .ProductCode = root.get<std::uint32_t>("ProductCode")
        //     });

        read_pdo_dictionary(root.get_object("PDO"));
        read_sdo_dictionary(root.get_object("SDO"));
    }

}
