#pragma once

#include <eng/sibus/node.hpp>
#include <common/lockers.hpp>

class lock_flags final
    : public eng::sibus::node
{
    std::vector<ambit::lock_system::subsystem_t> subsystems_;
    std::vector<ambit::lock_system::flag_locker_t> lockers_;

public:

    lock_flags(std::filesystem::path const &);

private:

    void update_locker(std::size_t, std::optional<bool>);

    void update_locker_status(std::size_t, ambit::lock_status);
};
