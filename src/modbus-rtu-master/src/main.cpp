#include "modbus-rtu-master.hpp"
#include "slaves/frequency-converter.hpp"
#include "slaves/PR200.hpp"

#include <eng/eng.hpp>
#include <eng/type-traits.hpp>
#include <eng/utils.hpp>
#include <eng/sibus/client.hpp>

// #include <print>

auto main(int argc, char *argv[]) -> int
{
    eng::sibus::client::init();

    // std::filesystem::path conf_path = std::filesystem::current_path();
    // if (argc > 1)
    //     conf_path = argv[1];
    //
    // std::println("LOAD MODBUS UNITS CONFIGURATIONS:");
    //
    // if (std::filesystem::is_directory(conf_path))
    // {
    //     for (auto const& dir_entry : std::filesystem::directory_iterator{ conf_path })
    //         new unit_node(dir_entry.path());
    // }
    // else
    // {
    //     new unit_node(conf_path);
    // }

    frequency_converter fc{ 10 };
    PR200 pr200{ 11 };

#ifdef BUILDROOT 
    modbus_rtu_master::init("/dev/ttyS0");
    // modbus_rtu_master::init("/dev/ttyUSB0");
#else
    modbus_rtu_master::init("/dev/ttySOCAT0");
#endif

    // Инициализация закончена,
    // запускаем цикл выполнения
    int result = eng::run();

    modbus_rtu_master::destroy();

    return result;
}


