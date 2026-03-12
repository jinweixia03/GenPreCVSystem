#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QString>

class QWidget;

namespace GenPreCVSystem {
namespace Utils {

// 前向声明
class AppException;

/**
 * @brief 统一的错误对话框工具类
 *
 * 提供静态方法显示各种类型的错误对话框
 */
class ErrorDialog
{
public:
    /**
     * @brief 显示严重错误对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     */
    static void showCritical(QWidget *parent, const QString &title, const QString &message);

    /**
     * @brief 显示警告对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     */
    static void showWarning(QWidget *parent, const QString &title, const QString &message);

    /**
     * @brief 显示信息对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     */
    static void showInformation(QWidget *parent, const QString &title, const QString &message);

    /**
     * @brief 显示异常对话框
     * @param parent 父窗口
     * @param e 异常对象
     */
    static void showException(QWidget *parent, const AppException &e);

    /**
     * @brief 显示确认对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     * @return 用户是否确认
     */
    static bool showConfirm(QWidget *parent, const QString &title, const QString &message);

    /**
     * @brief 显示带详细信息的错误对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 简短消息
     * @param details 详细信息
     */
    static void showCriticalWithDetails(QWidget *parent, const QString &title,
                                         const QString &message, const QString &details);

private:
    ErrorDialog() = delete;  // 禁止实例化
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // ERRORDIALOG_H
