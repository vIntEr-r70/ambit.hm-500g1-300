#include "slaves/frequency-converter.hpp"
#include "slaves/PR200.hpp"

#include <eng/eng.hpp>
#include <eng/type-traits.hpp>
#include <eng/utils.hpp>
#include <eng/sibus/client.hpp>

#if !defined(BUILDROOT) || defined(_WIN32)
    #include <eng/modbus/pipe-master.hpp>
#else
    #include <eng/modbus/rtu-master.hpp>
#endif

auto main(int argc, char *argv[]) -> int
{
    eng::sibus::client::init();

#if !defined(BUILDROOT) || defined(_WIN32)
    eng::modbus::pipe_master master0("ttyS0-10");
    frequency_converter fc{ 10 };
    master0.add_unit(fc);

    eng::modbus::pipe_master master1("ttyS0-11");
    PR200 pr200{ 11 };
    master1.add_unit(pr200);
#else
    eng::modbus::rtu_master master("/dev/ttyS0");

    frequency_converter fc{ 10 };
    master.add_unit(fc);

    PR200 pr200{ 11 };
    master.add_unit(pr200);
#endif


    // Инициализация закончена,
    // запускаем цикл выполнения
    return eng::run();
}


