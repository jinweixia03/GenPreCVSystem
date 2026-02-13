/**
 * @file appsettings.cpp
 * @brief 应用程序设置管理实现
 *
 * 使用 QSettings 持久化存储应用程序配置，包括：
 * - 默认打开/导出目录
 * - 最近文件列表
 * - YOLO 推理参数（置信度、IOU阈值等）
 * - 导出格式设置
 */

#include "appsettings.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

namespace GenPreCVSystem {
namespace Utils {

QSettings AppSettings::getSettings()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return QSettings(configPath + "/settings.ini", QSettings::IniFormat);
}

QString AppSettings::settingsFilePath()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return configPath + "/settings.ini";
}

// ========== 通用设置 ==========

QString AppSettings::defaultOpenDirectory()
{
    QSettings settings = getSettings();
    return settings.value("General/defaultOpenDirectory", QDir::homePath()).toString();
}

void AppSettings::setDefaultOpenDirectory(const QString &path)
{
    QSettings settings = getSettings();
    settings.setValue("General/defaultOpenDirectory", path);
    settings.sync();
}

QString AppSettings::defaultExportDirectory()
{
    QSettings settings = getSettings();
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return settings.value("General/defaultExportDirectory", defaultPath).toString();
}

void AppSettings::setDefaultExportDirectory(const QString &path)
{
    QSettings settings = getSettings();
    settings.setValue("General/defaultExportDirectory", path);
    settings.sync();
}

int AppSettings::maxRecentFiles()
{
    QSettings settings = getSettings();
    return settings.value("General/maxRecentFiles", 10).toInt();
}

void AppSettings::setMaxRecentFiles(int count)
{
    QSettings settings = getSettings();
    settings.setValue("General/maxRecentFiles", count);
    settings.sync();
}

bool AppSettings::autoSaveResults()
{
    QSettings settings = getSettings();
    return settings.value("General/autoSaveResults", false).toBool();
}

void AppSettings::setAutoSaveResults(bool enabled)
{
    QSettings settings = getSettings();
    settings.setValue("General/autoSaveResults", enabled);
    settings.sync();
}

// ========== YOLO 设置 ==========

QString AppSettings::pythonEnvironment()
{
    QSettings settings = getSettings();
    return settings.value("YOLO/pythonEnvironment", "").toString();
}

void AppSettings::setPythonEnvironment(const QString &path)
{
    QSettings settings = getSettings();
    settings.setValue("YOLO/pythonEnvironment", path);
    settings.sync();
}

float AppSettings::defaultConfThreshold()
{
    QSettings settings = getSettings();
    return settings.value("YOLO/defaultConfThreshold", 0.25f).toFloat();
}

void AppSettings::setDefaultConfThreshold(float value)
{
    QSettings settings = getSettings();
    settings.setValue("YOLO/defaultConfThreshold", value);
    settings.sync();
}

float AppSettings::defaultIOUThreshold()
{
    QSettings settings = getSettings();
    return settings.value("YOLO/defaultIOUThreshold", 0.45f).toFloat();
}

void AppSettings::setDefaultIOUThreshold(float value)
{
    QSettings settings = getSettings();
    settings.setValue("YOLO/defaultIOUThreshold", value);
    settings.sync();
}

int AppSettings::defaultImageSize()
{
    QSettings settings = getSettings();
    return settings.value("YOLO/defaultImageSize", 640).toInt();
}

void AppSettings::setDefaultImageSize(int size)
{
    QSettings settings = getSettings();
    settings.setValue("YOLO/defaultImageSize", size);
    settings.sync();
}

// ========== 导出设置 ==========

QString AppSettings::exportFormat()
{
    QSettings settings = getSettings();
    return settings.value("Export/format", "JSON").toString();
}

void AppSettings::setExportFormat(const QString &format)
{
    QSettings settings = getSettings();
    settings.setValue("Export/format", format);
    settings.sync();
}

bool AppSettings::includeTimestamp()
{
    QSettings settings = getSettings();
    return settings.value("Export/includeTimestamp", true).toBool();
}

void AppSettings::setIncludeTimestamp(bool enabled)
{
    QSettings settings = getSettings();
    settings.setValue("Export/includeTimestamp", enabled);
    settings.sync();
}

bool AppSettings::includeMetadata()
{
    QSettings settings = getSettings();
    return settings.value("Export/includeMetadata", true).toBool();
}

void AppSettings::setIncludeMetadata(bool enabled)
{
    QSettings settings = getSettings();
    settings.setValue("Export/includeMetadata", enabled);
    settings.sync();
}

// ========== 最近文件 ==========

QStringList AppSettings::recentFiles()
{
    QSettings settings = getSettings();
    return settings.value("RecentFiles/files").toStringList();
}

void AppSettings::setRecentFiles(const QStringList &files)
{
    QSettings settings = getSettings();
    settings.setValue("RecentFiles/files", files);
    settings.sync();
}

// ========== 工具方法 ==========

void AppSettings::resetToDefaults()
{
    QSettings settings = getSettings();

    // 通用设置
    settings.setValue("General/defaultOpenDirectory", QDir::homePath());
    settings.setValue("General/defaultExportDirectory",
                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    settings.setValue("General/maxRecentFiles", 10);
    settings.setValue("General/autoSaveResults", false);

    // YOLO 设置
    settings.setValue("YOLO/defaultConfThreshold", 0.25f);
    settings.setValue("YOLO/defaultIOUThreshold", 0.45f);
    settings.setValue("YOLO/defaultImageSize", 640);

    // 导出设置
    settings.setValue("Export/format", "JSON");
    settings.setValue("Export/includeTimestamp", true);
    settings.setValue("Export/includeMetadata", true);

    settings.sync();
}

} // namespace Utils
} // namespace GenPreCVSystem
