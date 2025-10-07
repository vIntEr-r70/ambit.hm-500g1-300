#include <QApplication>
#include <QTextCodec>
#include <QFile>
#include <QFontDatabase>
#include <QScreen>

#include "gui/MainFrame.h"

#include <global.h>

#include <aem/log.h>
#include <aem/aem.h>
#include <aem/timer.h>

int main(int argc, char *argv[])
{
    global::create();

    QApplication a(argc, argv);
    a.setStyle("plastique");
    a.setDoubleClickInterval(500);

    QCoreApplication::setOrganizationName("ambit");
    QCoreApplication::setApplicationName("cnc-ctrl");

    QLocale::setDefault(QLocale::C);

    QFile qss("StyleSheet.qss");
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QByteArray block = qss.readAll();
        block.append('\0');
        a.setStyleSheet(block.constData());
    }

    // int id = QFontDatabase::addApplicationFont("./Shentox-Regular.ttf");
    // int id = QFontDatabase::addApplicationFont("./Manrope-Regular.ttf");
    int id = QFontDatabase::addApplicationFont("./dejavu/DejaVuSans.ttf");
    // if (id < 0)
    //     aem::log::error("Не удалось загрузить шрифт....");
    // else
    //     QApplication::setFont(QFontDatabase::applicationFontFamilies(id).at(0));

#ifdef BUILDROOT
	QScreen* screen = QGuiApplication::primaryScreen();
	QRect screenGeometry(screen->geometry());
#else
	QRect screenGeometry(0, 0, 1024, 768);
#endif

    MainFrame *w = new MainFrame();
    w->setFixedSize(screenGeometry.width(), screenGeometry.height());
	w->show();

    aem::timer timer;
    timer.timeout = [&]
    {
        a.processEvents();

        if (!w->isHidden())
            return;

        // Закрываем соединение с агентом
        global::rpc().close();
        global::nf().close();

        // Останавливаем обработку очереди сообщении интерфейса
        timer.stop();
    };
    timer.repeat(aem::time_span::ms(10));

    return aem::run([w](int result)
        {
            // Вызываем деструкторы окон
            delete w;

            global::destroy();
            return result;
        });
}

