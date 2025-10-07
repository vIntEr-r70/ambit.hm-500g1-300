#pragma once

#include <aem/net/base_tcp.h>
#include <aem/json.hpp>

#include <functional>

class db_model
    : private aem::net::base_tcp_client
{
public:

    std::function<void(std::string_view)> on_error;
    std::function<void(nlohmann::json const&)> on_done;

public:

    db_model(std::string_view, aem::uint16) noexcept;

public:

    static void make_spec(std::string_view, aem::buffer&) noexcept;

    static void make_request(char, std::string_view, aem::buffer&) noexcept;

    static bool parse_response(aem::uint8 const*, aem::uint8 const*, nlohmann::json&, std::string&) noexcept;

public:

    void create_table_if_not_exists();

    void create_new_archive(std::string const&, std::string const&) noexcept;

    void delete_archive(std::size_t) noexcept;

    void update_archive(std::size_t, aem::int64) noexcept;

    void finalize_archive(std::size_t, std::size_t) noexcept;

private:

    void create_new_archive_request() noexcept;

    void create_new_archive_response(aem::uint8 const*, aem::uint8 const*) noexcept;

    void get_in_proc_archive_request() noexcept;

    void get_in_proc_archive_response(aem::uint8 const*, aem::uint8 const*) noexcept;

    void delete_archive_request() noexcept;

    void delete_archive_response(aem::uint8 const*, aem::uint8 const*) noexcept;

    void update_archive_request() noexcept;

    void update_archive_response(aem::uint8 const*, aem::uint8 const*) noexcept;

private:


	void recv_data() noexcept override final;

	void connection_closed() noexcept override final;

    void connection_success() noexcept override final;

    void connection_error(std::string_view) noexcept override final;

private:

    void (db_model::*request_)();
    void (db_model::*response_)(aem::uint8 const*, aem::uint8 const*);

    std::string host_;
    aem::uint16 port_;

    std::string sql_;

    std::string emsg_;
    nlohmann::json json_;
};


