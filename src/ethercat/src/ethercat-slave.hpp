#pragma once

#include <cstdint>

// Базовый класс для реализации модулей,
// способных работать с драйвером ethercat

struct slave_target_t
{
    std::uint16_t alias;
    std::uint16_t position;
};

struct slave_info_t
{
    slave_target_t target;
    std::uint32_t VendorID;
    std::uint32_t ProductCode;
};

class ethercat_slave
{
    ethercat_slave(ethercat_slave &&) = delete;
    ethercat_slave& operator=(ethercat_slave &&) = delete;
    ethercat_slave(ethercat_slave const &) = delete;
    ethercat_slave& operator=(ethercat_slave const &) = delete;

    slave_info_t slave_info_;

public:

    virtual ~ethercat_slave() = default;

protected:

    ethercat_slave(slave_info_t slave_info)
        : slave_info_(slave_info)
    { }

public:

    virtual void update(double) = 0;

public:

    slave_info_t info() const noexcept { return slave_info_; }
};
