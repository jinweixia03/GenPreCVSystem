#ifndef PYTHONENVIRONMENT_H
#define PYTHONENVIRONMENT_H

#include <QString>
#include <QVector>

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief Python 环境信息
 *
 * 用于 DL 和 CL-PSDD 服务的公共结构体
 */
struct PythonEnvironment {
    QString name;           // 环境名称
    QString path;           // Python 可执行文件路径
    QString type;           // 环境类型: "conda", "venv", "system"
    bool hasUltralytics;    // 是否安装了 ultralytics
    bool hasTorch;          // 是否安装了 PyTorch

    PythonEnvironment()
        : hasUltralytics(false), hasTorch(false) {}
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // PYTHONENVIRONMENT_H
