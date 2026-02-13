#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>

class QSettings;

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 应用程序设置管理类
 *
 * 提供统一的设置访问接口，使用 QSettings 持久化存储
 */
class AppSettings
{
public:
    // ========== 通用设置 ==========

    /**
     * @brief 获取默认打开目录
     */
    static QString defaultOpenDirectory();

    /**
     * @brief 设置默认打开目录
     */
    static void setDefaultOpenDirectory(const QString &path);

    /**
     * @brief 获取默认导出目录
     */
    static QString defaultExportDirectory();

    /**
     * @brief 设置默认导出目录
     */
    static void setDefaultExportDirectory(const QString &path);

    /**
     * @brief 获取最近文件最大数量
     */
    static int maxRecentFiles();

    /**
     * @brief 设置最近文件最大数量
     */
    static void setMaxRecentFiles(int count);

    /**
     * @brief 获取是否自动保存结果
     */
    static bool autoSaveResults();

    /**
     * @brief 设置是否自动保存结果
     */
    static void setAutoSaveResults(bool enabled);

    // ========== YOLO 设置 ==========

    /**
     * @brief 获取 Python 环境路径
     */
    static QString pythonEnvironment();

    /**
     * @brief 设置 Python 环境路径
     */
    static void setPythonEnvironment(const QString &path);

    /**
     * @brief 获取默认置信度阈值
     */
    static float defaultConfThreshold();

    /**
     * @brief 设置默认置信度阈值
     */
    static void setDefaultConfThreshold(float value);

    /**
     * @brief 获取默认 IOU 阈值
     */
    static float defaultIOUThreshold();

    /**
     * @brief 设置默认 IOU 阈值
     */
    static void setDefaultIOUThreshold(float value);

    /**
     * @brief 获取默认图像尺寸
     */
    static int defaultImageSize();

    /**
     * @brief 设置默认图像尺寸
     */
    static void setDefaultImageSize(int size);

    // ========== 导出设置 ==========

    /**
     * @brief 获取导出格式 (JSON/CSV)
     */
    static QString exportFormat();

    /**
     * @brief 设置导出格式
     */
    static void setExportFormat(const QString &format);

    /**
     * @brief 获取是否在导出时包含时间戳
     */
    static bool includeTimestamp();

    /**
     * @brief 设置是否在导出时包含时间戳
     */
    static void setIncludeTimestamp(bool enabled);

    /**
     * @brief 获取是否在导出时包含元数据
     */
    static bool includeMetadata();

    /**
     * @brief 设置是否在导出时包含元数据
     */
    static void setIncludeMetadata(bool enabled);

    // ========== 最近文件 ==========

    /**
     * @brief 获取最近文件列表
     */
    static QStringList recentFiles();

    /**
     * @brief 设置最近文件列表
     */
    static void setRecentFiles(const QStringList &files);

    // ========== 工具方法 ==========

    /**
     * @brief 重置所有设置为默认值
     */
    static void resetToDefaults();

    /**
     * @brief 获取设置文件路径
     */
    static QString settingsFilePath();

private:
    static QSettings getSettings();
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // APPSETTINGS_H
