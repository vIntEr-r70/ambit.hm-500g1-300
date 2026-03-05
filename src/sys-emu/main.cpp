#include "main-frame.hpp"
#include "listeners/syslink.hpp"

#include "listeners/devices/PLC110.hpp"
#include "listeners/devices/FC.hpp"
#include "listeners/devices/PR200.hpp"
#include "listeners/devices/PR205-A1.hpp"
#include "listeners/devices/PR205-A2.hpp"
#include "listeners/devices/PR205-A14.hpp"
#include "listeners/devices/PR205-B2.hpp"
#include "listeners/devices/PR205-B6.hpp"
#include "listeners/devices/PR205-B10.hpp"

#include <QApplication>
#include <QTextCodec>
#include <QStyleFactory>

#include <eng/eng.hpp>
#include <eng/timer.hpp>
#include <eng/log.hpp>

#ifdef _WIN32
    #include <windows.h>
#endif

auto main(int argc, char *argv[]) -> int
{
#ifdef _WIN32
    // Устанавливаем UTF-8 для вывода в консоль
    SetConsoleOutputCP(CP_UTF8);
    // Устанавливаем UTF-8 для ввода (если нужно)
    SetConsoleCP(CP_UTF8);
#endif

    QApplication a(argc, argv);
    a.setStyle("plastique");
    QLocale::setDefault(QLocale::C);
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    syslink::add_new_device<devices::PLC110>();
    syslink::add_new_device<devices::FC>();
    syslink::add_new_device<devices::PR200>();
    syslink::add_new_device<devices::PR205_A1>();
    syslink::add_new_device<devices::PR205_A2>();
    syslink::add_new_device<devices::PR205_A14>();
    syslink::add_new_device<devices::PR205_B2>();
    syslink::add_new_device<devices::PR205_B6>();
    syslink::add_new_device<devices::PR205_B10>();

    main_frame *w = new main_frame();
    QRect screenGeometry(0, 0, 1024, 768);
    w->setFixedSize(screenGeometry.width(), screenGeometry.height());
    w->show();

    eng::timer::id_t timer_id = eng::timer::add_ms(10, [&]
    {
        a.processEvents();

        if (!w->isHidden())
            return;

        eng::timer::kill_timer(timer_id);
    });

    int result = eng::run();

    // Вызываем деструкторы окон
    delete w;

    return result;
}

