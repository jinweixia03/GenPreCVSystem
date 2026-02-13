#include "ui/mainwindow.h"
#include "utils/exceptions.h"
#include "utils/errordialog.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 日志文件路径
static QString logFilePath;
static QFile logFile;

/**
 * @brief 自定义消息处理器
 *
 * 捕获 Qt 消息并写入日志文件
 */
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    // 格式化时间戳
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 根据消息类型添加前缀
    QString typeStr;
    switch (type) {
    case QtDebugMsg:
        typeStr = "DEBUG";
        break;
    case QtInfoMsg:
        typeStr = "INFO";
        break;
    case QtWarningMsg:
        typeStr = "WARN";
        break;
    case QtCriticalMsg:
        typeStr = "ERROR";
        break;
    case QtFatalMsg:
        typeStr = "FATAL";
        break;
    }

    // 格式化日志消息
    QString logMessage = QString("[%1] [%2] %3")
        .arg(timestamp)
        .arg(typeStr)
        .arg(msg);

    // 写入日志文件
    if (logFile.isOpen()) {
        QTextStream stream(&logFile);
        stream.setEncoding(QStringConverter::Utf8);
        stream << logMessage << "\n";
        stream.flush();
    }

    // 同时输出到控制台
    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());

    // 致命错误时终止程序
    if (type == QtFatalMsg) {
        abort();
    }
}

/**
 * @brief 初始化日志系统
 */
void initLogging()
{
    // 获取日志目录
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 创建日志文件
    logFilePath = logDir + "/GenPreCVSystem.log";
    logFile.setFileName(logFilePath);

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qInstallMessageHandler(customMessageHandler);
        qInfo() << "===== Application Started =====";
        qInfo() << "Log file:" << logFilePath;
    } else {
        qWarning() << "Failed to open log file:" << logFilePath;
    }
}

/**
 * @brief 清理日志系统
 */
void cleanupLogging()
{
    qInfo() << "===== Application Closed =====";
    if (logFile.isOpen()) {
        logFile.close();
    }
}

/**
 * @brief 全局异常处理器
 *
 * 显示错误对话框并记录日志
 */
void handleException(const std::exception &e)
{
    qCritical() << "Unhandled exception:" << e.what();

    // 尝试转换为应用异常
    try {
        const GenPreCVSystem::Utils::AppException* appEx =
            dynamic_cast<const GenPreCVSystem::Utils::AppException*>(&e);

        if (appEx) {
            GenPreCVSystem::Utils::ErrorDialog::showException(nullptr, *appEx);
        } else {
            GenPreCVSystem::Utils::ErrorDialog::showCritical(
                nullptr,
                QObject::tr("应用程序错误"),
                QString::fromLocal8Bit(e.what())
            );
        }
    } catch (...) {
        // 如果错误对话框也失败，使用简单的消息框
        QMessageBox::critical(nullptr,
            QObject::tr("应用程序错误"),
            QString::fromLocal8Bit(e.what()));
    }
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 设置Windows控制台代码页为UTF-8，解决中文乱码问题
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 初始化日志系统
    initLogging();

    int result = -1;

    try {
        // Qt 6 中高 DPI 缩放已默认启用，无需手动设置
        QApplication a(argc, argv);

        // 设置应用程序信息
        QApplication::setApplicationName("GenPreCVSystem");
        QApplication::setApplicationVersion("1.0.0");
        QApplication::setOrganizationName("GenPreCV");

        // 安装翻译器
        QTranslator translator;
        const QStringList uiLanguages = QLocale::system().uiLanguages();
        for (const QString &locale : uiLanguages) {
            const QString baseName = "GenPreCVSystem_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName)) {
                a.installTranslator(&translator);
                break;
            }
        }

        // 创建主窗口
        MainWindow w;
        w.show();

        // 运行事件循环
        result = a.exec();

    } catch (const GenPreCVSystem::Utils::AppException &e) {
        qCritical() << "AppException caught in main:" << e.fullMessage();
        handleException(e);
        result = -1;
    } catch (const std::exception &e) {
        qCritical() << "std::exception caught in main:" << e.what();
        handleException(e);
        result = -1;
    } catch (...) {
        qCritical() << "Unknown exception caught in main";
        QMessageBox::critical(nullptr,
            QObject::tr("应用程序错误"),
            QObject::tr("发生未知错误，程序即将退出。"));
        result = -1;
    }

    // 清理日志系统
    cleanupLogging();

    return result;
}
