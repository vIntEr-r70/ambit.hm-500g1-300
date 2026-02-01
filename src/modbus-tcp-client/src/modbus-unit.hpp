#pragma once

#include <string_view>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <optional>

class modbus_unit
{
    std::string host_;
    std::uint16_t port_;
    std::uint8_t id_{ 0 };

    std::size_t idx_;

    struct read_holding_t
    {
        std::uint16_t address;
        std::size_t number_of;
        std::uint32_t msecs;
    };
    std::vector<read_holding_t> read_holding_;

    std::vector<std::uint16_t> result_;

    std::optional<bool> online_{ false };

protected:

    using readed_regs_t = std::span<std::uint16_t const>;

protected:

    modbus_unit(std::string_view, std::uint16_t, std::uint8_t = 0);

public:

    virtual ~modbus_unit() = default;

public:

    std::string const &host() const noexcept { return host_; }

    std::uint16_t port() const noexcept { return port_; }

    std::uint8_t id() const noexcept { return id_; }

    template <typename F>
    void for_each_read_holding(F f)
    {
        for (std::size_t idx = 0; idx < read_holding_.size(); ++idx)
        {
            auto const &record = read_holding_[idx];
            f(idx, record.address, record.number_of, record.msecs);
        }
    }

    void read_holding_done(std::size_t, std::span<std::uint8_t const>);

    void write_done();

    void connected();

    void disconnected();

protected:

    bool is_online() const noexcept { return online_ ? *online_ : false; }

    std::size_t add_read_task(std::uint16_t, std::size_t, std::uint32_t);

    void write_single(std::uint16_t, std::uint16_t);

private:

    virtual void read_task_done(std::size_t, readed_regs_t) { }

    virtual void write_task_done(std::size_t) { }

    virtual void connection_was_lost() = 0;
};

