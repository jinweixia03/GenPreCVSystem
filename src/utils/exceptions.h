#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <QString>
#include <QDateTime>

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 应用程序基础异常类
 *
 * 所有自定义异常的基类，提供统一的错误信息接口
 */
class AppException : public std::exception
{
public:
    /**
     * @brief 构造函数
     * @param message 错误消息
     * @param context 错误上下文（可选，如函数名或模块名）
     */
    explicit AppException(const QString &message, const QString &context = QString());

    /**
     * @brief 获取错误消息（C风格字符串）
     */
    const char* what() const noexcept override;

    /**
     * @brief 获取错误消息（Qt字符串）
     */
    QString message() const;

    /**
     * @brief 获取错误上下文
     */
    QString context() const;

    /**
     * @brief 获取完整错误信息（包含上下文）
     */
    QString fullMessage() const;

    /**
     * @brief 获取时间戳
     */
    QDateTime timestamp() const;

protected:
    QString m_message;
    QString m_context;
    QDateTime m_timestamp;
    mutable QByteArray m_whatBuffer;
};

/**
 * @brief 服务异常
 *
 * 用于 YOLO 服务、图像处理服务等外部服务相关错误
 */
class ServiceException : public AppException
{
public:
    explicit ServiceException(const QString &message, const QString &serviceName = QString())
        : AppException(message, serviceName.isEmpty() ? "Service" : QString("Service[%1]").arg(serviceName))
        , m_serviceName(serviceName)
    {}

    QString serviceName() const { return m_serviceName; }

private:
    QString m_serviceName;
};

/**
 * @brief 文件操作异常
 *
 * 用于文件读写、路径操作相关错误
 */
class FileException : public AppException
{
public:
    explicit FileException(const QString &message, const QString &filePath = QString())
        : AppException(message, filePath.isEmpty() ? "File" : QString("File[%1]").arg(filePath))
        , m_filePath(filePath)
    {}

    QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
};

/**
 * @brief 模型加载异常
 *
 * 用于 YOLO 模型加载、推理相关错误
 */
class ModelException : public AppException
{
public:
    explicit ModelException(const QString &message, const QString &modelPath = QString())
        : AppException(message, modelPath.isEmpty() ? "Model" : QString("Model[%1]").arg(modelPath))
        , m_modelPath(modelPath)
    {}

    QString modelPath() const { return m_modelPath; }

private:
    QString m_modelPath;
};

/**
 * @brief 图像处理异常
 *
 * 用于图像加载、处理、转换相关错误
 */
class ImageProcessException : public AppException
{
public:
    explicit ImageProcessException(const QString &message, const QString &operation = QString())
        : AppException(message, operation.isEmpty() ? "ImageProcess" : QString("ImageProcess[%1]").arg(operation))
        , m_operation(operation)
    {}

    QString operation() const { return m_operation; }

private:
    QString m_operation;
};

/**
 * @brief 配置异常
 *
 * 用于配置文件、设置相关错误
 */
class ConfigException : public AppException
{
public:
    explicit ConfigException(const QString &message, const QString &configKey = QString())
        : AppException(message, configKey.isEmpty() ? "Config" : QString("Config[%1]").arg(configKey))
        , m_configKey(configKey)
    {}

    QString configKey() const { return m_configKey; }

private:
    QString m_configKey;
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // EXCEPTIONS_H
