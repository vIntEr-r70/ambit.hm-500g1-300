#pragma once

#include <eng/sibus/node.hpp>

#include <filesystem>

class intervals final
    : public eng::sibus::node
{
    enum class lstatus : std::uint8_t
    {
        normal,
        warning,
        critical
    };

    struct subsystem_t
    {
        std::string name;
        eng::sibus::output_port_id_t port;
        std::vector<std::size_t> lockers;
        std::optional<bool> locked;
    };
    std::vector<subsystem_t> subsystems_;

    struct locker_t
    {
        std::string name;
        eng::sibus::output_port_id_t port;
        std::vector<std::size_t> subsystems;
        std::optional<lstatus> status;
        double value;
        std::array<double, 4> range;
    };
    std::vector<locker_t> lockers_;

public:

    intervals(std::filesystem::path const &);

private:

    void update_locker(std::size_t, double);

    void update_locker_status(std::size_t, lstatus);

    void update_subsystem(std::size_t);

private:

    bool load_config(std::filesystem::path const &);

    void configure_relationship(std::size_t, std::uint32_t);
};
