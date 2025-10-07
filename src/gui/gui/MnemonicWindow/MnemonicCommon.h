#pragma once

#include <array>

#include <QPixmap>

struct MnemonicImages final
    : public std::array<QPixmap, 13>
{
    enum : int
    {
       valve_closed,
       valve_opened,
       faucet,
       rfaucet,
       shower_off,
       shower_half,
       shower_max,
       slevel_on,
       slevel_off,
       heater_on,
       heater_off,
       pump_on,
       pump_off,
    };
};

struct MnemonicInfo final 
{
    struct desc
    {
        QString units;
        QString title;
        QColor color;
    };

    static desc const& pressure() noexcept 
    {
        static desc const value{ "Бар", "Датчик\nдавления", "#F56D21" };
        return value; 
    }

    static desc const& temperature() noexcept 
    {
        static desc const value{ "°С", "Датчик\nтемпературы", "#29AC39" };
        return value; 
    }

    static desc const& inflow() noexcept 
    {
        static desc const value{ "л/мин", "Датчик\nпротока", "#1EA0FF" };
        return value; 
    }
};
