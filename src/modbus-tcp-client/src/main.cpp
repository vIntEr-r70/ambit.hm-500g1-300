#include "unit-node.hpp"
#include "units-interaction.hpp"
#include "units/PR205-A1.hpp"
#include "units/PR205-A2.hpp"
#include "units/PR205-BN.hpp"
#include "units/PR205-A14.hpp"
#include "units/PLC110.hpp"

#include <eng/eng.hpp>
#include <eng/type-traits.hpp>
#include <eng/utils.hpp>
#include <eng/sibus/client.hpp>
#include <eng/log.hpp>

auto main() -> int
{
    // char const *LIAEM_RO_PATH = std::getenv("LIAEM_RO_PATH");
    // if (!LIAEM_RO_PATH)
    // {
    //     eng::log::error("no LIAEM_RO_PATH enviroment variable");
    //     return 1;
    // }
    //
    // std::filesystem::path conf_path(LIAEM_RO_PATH);
    // conf_path /= "modbus-tcp-units";
    //
    // if (!std::filesystem::exists(conf_path))
    // {
    //     eng::log::info("modbus-tcp-client: units list empty");
    //     return 0;
    // }
    //
    // eng::log::info("LOAD MODBUS UNITS CONFIGURATIONS: {}", conf_path.native());
    //
    // for (auto const& dir_entry : std::filesystem::directory_iterator{ conf_path })
    //     new unit_node(dir_entry.path());

#ifdef BUILDROOT

    PLC110 plc110("10.0.0.10", 502);

    PR205_A1  pr205_a1 ("10.0.0.11", 502);
    PR205_A2  pr205_a2 ("10.0.0.12", 502);
    PR205_A14 pr205_a14("10.0.0.16", 502);

    PR205_BN pr205_b2 (2,  "10.0.0.13", 502);
    PR205_BN pr205_b6 (6,  "10.0.0.14", 502);
    PR205_BN pr205_b10(10, "10.0.0.15", 502);

#else

    PLC110 plc110("127.0.0.1", 1200);

    PR205_A1  pr205_a1 ("127.0.0.1", 1200);
    PR205_A2  pr205_a2 ("127.0.0.1", 1200);
    PR205_A14 pr205_a14("127.0.0.1", 1200);

    PR205_BN pr205_b2 (2,  "127.0.0.1", 1200);
    PR205_BN pr205_b6 (6,  "127.0.0.1", 1200);
    PR205_BN pr205_b10(10, "127.0.0.1", 1200);

#endif

    eng::sibus::client::init();

    // Устанавливаем и поддерживаем соединения с устройствами
    units_interaction::start();

    // Инициализация закончена,
    // запускаем цикл выполнения
    return eng::run();
}


