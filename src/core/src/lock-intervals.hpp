#pragma once

#include <eng/sibus/node.hpp>
#include <common/lockers.hpp>

class lock_intervals final
    : public eng::sibus::node
{
    std::vector<ambit::lock_system::subsystem_t> subsystems_;
    std::vector<ambit::lock_system::value_locker_t> lockers_;

public:

    lock_intervals(std::filesystem::path const &);

private:

    void update_locker(std::size_t, double);

    void update_locker_status(std::size_t, ambit::lock_status);
};
