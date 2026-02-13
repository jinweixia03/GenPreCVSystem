#include "exceptions.h"

namespace GenPreCVSystem {
namespace Utils {

AppException::AppException(const QString &message, const QString &context)
    : m_message(message)
    , m_context(context)
    , m_timestamp(QDateTime::currentDateTime())
{
}

const char* AppException::what() const noexcept
{
    // 缓存完整消息以便返回 C 风格字符串
    m_whatBuffer = fullMessage().toUtf8();
    return m_whatBuffer.constData();
}

QString AppException::message() const
{
    return m_message;
}

QString AppException::context() const
{
    return m_context;
}

QString AppException::fullMessage() const
{
    if (m_context.isEmpty()) {
        return m_message;
    }
    return QString("[%1] %2").arg(m_context, m_message);
}

QDateTime AppException::timestamp() const
{
    return m_timestamp;
}

} // namespace Utils
} // namespace GenPreCVSystem
