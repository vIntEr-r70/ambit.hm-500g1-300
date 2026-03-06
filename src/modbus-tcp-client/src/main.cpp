#include "units/PR205-A1.hpp"
#include "units/PR205-A2.hpp"
#include "units/PR205-BN.hpp"
#include "units/PR205-A14.hpp"
#include "units/PLC110.hpp"

#include <eng/eng.hpp>
#include <eng/sibus/client.hpp>

#if !defined(BUILDROOT) || defined(_WIN32)
    #include <eng/modbus/pipe-master.hpp>
#else
    #include <eng/modbus/tcp-master.hpp>
#endif

auto main() -> int
{
    eng::sibus::client::init();

#if !defined(BUILDROOT) || defined(_WIN32)
    eng::modbus::pipe_master plc100("10.0.0.10");
    eng::modbus::pipe_master pr205_a1("10.0.0.11");
    eng::modbus::pipe_master pr205_a2("10.0.0.12");
    eng::modbus::pipe_master pr205_a14("10.0.0.16");
    eng::modbus::pipe_master pr205_b2("10.0.0.13");
    eng::modbus::pipe_master pr205_b6("10.0.0.14");
    eng::modbus::pipe_master pr205_b10("10.0.0.15");
#else
    eng::modbus::tcp_master plc100("10.0.0.10", 502);
    eng::modbus::tcp_master pr205_a1("10.0.0.11", 502);
    eng::modbus::tcp_master pr205_a2("10.0.0.12", 502);
    eng::modbus::tcp_master pr205_a14("10.0.0.16", 502);
    eng::modbus::tcp_master pr205_b2("10.0.0.13", 502);
    eng::modbus::tcp_master pr205_b6("10.0.0.14", 502);
    eng::modbus::tcp_master pr205_b10("10.0.0.15", 502);
#endif

    PLC110 unit_plc110(100);
    plc100.add_unit(unit_plc110);

    PR205_A1 unit_pr205_a1(0);
    pr205_a1.add_unit(unit_pr205_a1);

    PR205_A2 unit_pr205_a2(0);
    pr205_a2.add_unit(unit_pr205_a2);

    PR205_A14 unit_pr205_a14(0);
    pr205_a14.add_unit(unit_pr205_a14);

    PR205_BN unit_pr205_b2(0, 2);
    pr205_b2.add_unit(unit_pr205_b2);

    PR205_BN unit_pr205_b6(0, 6);
    pr205_b6.add_unit(unit_pr205_b6);

    PR205_BN unit_pr205_b10(0, 10);
    pr205_b10.add_unit(unit_pr205_b10);

    // Инициализация закончена,
    // запускаем цикл выполнения
    return eng::run();
}


