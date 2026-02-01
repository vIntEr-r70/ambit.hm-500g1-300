#pragma once

#include "unit-conf-loader.hpp"

#include <eng/type-traits.hpp>
#include <eng/sibus/node.hpp>

#include <filesystem>

// Список записей конфигураций устройств
struct unit_conf_record_t
{
    std::string name;

    std::size_t number_of;
    unit_conf_loader::record_t record;

    eng::type_traits::type_id_t type_id;

    eng::sibus::output_port_id_t port_id;
    std::vector<std::uint8_t> memory;

    bool invalidated{ true };
};

class unit_node final
    : public eng::sibus::node
{

    std::string host_;
    std::uint16_t port_;

    std::vector<unit_conf_record_t> records_;

public:

    unit_node(std::filesystem::path const &);

private:

    bool read_unit_conf_file(std::string_view, std::filesystem::path const &);

    void on_modbus_unit_data_changed(std::size_t, std::span<std::uint8_t const>);
};
