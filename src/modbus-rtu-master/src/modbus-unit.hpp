#pragma once

#include <eng/timer.hpp>

#include <cstdint>
#include <span>
#include <vector>
#include <optional>

class modbus_unit
{
    std::uint8_t id_{ 0 };

    struct read_holding_t
    {
        std::uint16_t address;
        std::size_t number_of;
        std::uint32_t msecs;
    };
    std::vector<read_holding_t> read_holding_;

    std::vector<std::uint16_t> result_;

    std::optional<bool> online_;

    eng::timer::id_t offline_timer_id_;

protected:

    using readed_regs_t = std::span<std::uint16_t const>;

protected:

    modbus_unit(std::uint8_t);

public:

    virtual ~modbus_unit() = default;

public:

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

    void write_done(std::size_t);

    void offline();

protected:

    bool is_online() const noexcept { return online_ ? *online_ : false; }

    std::size_t add_read_task(std::uint16_t, std::size_t, std::uint32_t);

    std::size_t write_single(std::uint16_t, std::uint16_t);

    std::size_t write_multiple(std::uint16_t, std::span<std::uint16_t const>);

private:

    void online(bool);

private:

    virtual void read_task_done(std::size_t, readed_regs_t) { }

    virtual void write_task_done(std::size_t) { }

    virtual void now_unit_online() { }

    virtual void connection_was_lost() { }
};
