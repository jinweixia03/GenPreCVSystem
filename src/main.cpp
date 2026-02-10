#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 设置Windows控制台代码页为UTF-8，解决中文乱码问题
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // Qt 6 中高 DPI 缩放已默认启用，无需手动设置
    QApplication a(argc, argv);

    // 设置应用程序信息
    QApplication::setApplicationName("GenPreCVSystem");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("GenPreCV");

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "GenPreCVSystem_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.show();

    return a.exec();
}
