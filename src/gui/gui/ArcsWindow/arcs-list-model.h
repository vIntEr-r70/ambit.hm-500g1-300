#pragma once

#include <QAbstractTableModel>

#include <aem/net/base_tcp.h>
#include <aem/json.hpp>

class arcs_list_model 
    : public QAbstractTableModel
    , private aem::net::base_tcp_client
{
public:

    arcs_list_model() noexcept;

public:

    void loadPage(std::size_t, std::size_t) noexcept;

public:

    QString const& get_work_desc(int) const noexcept;

    QString const& get_cg_name(int) const noexcept;

    QString const& get_cg_type(int) const noexcept;

    QString const& get_ab_num(int) const noexcept;

    QString const& get_zrk_num(int) const noexcept;

    QString const& get_user_name(int) const noexcept;

    QString const& get_arc_name(int) const noexcept;

    std::size_t get_arc_id(int) const noexcept;

    void close_model() noexcept { base_tcp_client::close(); }

private:

    int rowCount(QModelIndex const&) const noexcept override final;

    int columnCount(QModelIndex const&) const noexcept override final;

    QVariant data(QModelIndex const&, int) const noexcept override final;

    QVariant headerData(int, Qt::Orientation, int) const noexcept override final;

private:

	void recv_data() noexcept override final;

	void connection_closed() noexcept override final;

    void connection_success() noexcept override final;

    void connection_error(std::string_view) noexcept override final;

private:

    void response_count(aem::uint8 const*, aem::uint8 const*) noexcept;

    void response_select(aem::uint8 const*, aem::uint8 const*) noexcept;

private:

    bool is_row_in_data(int) const noexcept;

private:

    QStringList header_;
    QString sstub_;

    std::string select_sql_;

    std::size_t offset_{ 0 };
    std::size_t next_offset_{ 0 };
    constexpr static std::size_t limit_{ 100 };
    std::array<std::array<QString, 10>, limit_> data_;
    std::array<std::size_t, limit_> rid_;
    std::size_t data_real_size_{ 0 };
    std::size_t arcs_count_{ 0 };

    std::size_t unhandled_page_min_{ 0 };
    std::size_t unhandled_page_max_{ 0 };

    void(arcs_list_model::*on_response_)(aem::uint8 const*, aem::uint8 const*) noexcept;
};

