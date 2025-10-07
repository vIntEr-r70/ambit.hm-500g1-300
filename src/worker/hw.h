#pragma once

#include <hardware.h>

class hw
    : public we::hardware
{
private:

    hw() noexcept;

public:

    static void create() noexcept;

    static void destroy() noexcept;

    static hw& ref() noexcept { return *instance_; }

private:

    we::controller_creator get_controller_creator(std::string_view) override final;

private:

    static hw *instance_;
};
