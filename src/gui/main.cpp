#include <QApplication>
#include <QTextCodec>
#include <QFile>
#include <QFontDatabase>
#include <QScreen>
#include <QStyleFactory>

#include "gui/MainFrame.h"

#include <eng/eng.hpp>
#include <eng/timer.hpp>
#include <eng/sibus/client.hpp>
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
    a.setDoubleClickInterval(500);

    QLocale::setDefault(QLocale::C);

    QCoreApplication::setOrganizationName("ambit");
    QCoreApplication::setApplicationName("cnc-ctrl");

    QApplication::setStyle(QStyleFactory::create("Fusion"));

#ifndef _WIN32
    int id = QFontDatabase::addApplicationFont("Roboto-Regular.ttf");
    if (id < 0)
        eng::log::error("Не удалось загрузить шрифт....");
    else
        QApplication::setFont(QFontDatabase::applicationFontFamilies(id).at(0));
#endif

#if defined(BUILDROOT) && !defined(_WIN32)
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry(screen->geometry());
#else
    QRect screenGeometry(0, 0, 1024, 768);
#endif

    MainFrame *w = new MainFrame();
    w->setFixedSize(screenGeometry.width(), screenGeometry.height());
    w->show();

    eng::sibus::client::init();

    eng::timer::id_t timer_id = eng::timer::add_ms(10, [&]
    {
        a.processEvents();

        if (!w->isHidden())
            return;

        eng::timer::kill_timer(timer_id);

        eng::sibus::client::destroy();
    });

    int result = eng::run();

    // Вызываем деструкторы окон
    delete w;

    return result;
}

