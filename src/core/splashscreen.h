#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace GenPreCVSystem {
namespace UI {

/**
 * @brief 启动画面窗口
 *
 * 在应用程序启动时显示的现代化深色风格启动画面
 * 显示 Logo、应用名称、加载进度和版本信息
 */
class SplashScreen : public QWidget
{
    Q_OBJECT

public:
    explicit SplashScreen(QWidget *parent = nullptr);
    ~SplashScreen();

    /**
     * @brief 设置加载进度
     * @param value 进度值 (0-100)
     */
    void setProgress(int value);

    /**
     * @brief 设置加载状态文本
     * @param status 状态文本
     */
    void setStatus(const QString &status);

    /**
     * @brief 设置版本信息
     * @param version 版本字符串
     */
    void setVersion(const QString &version);

    /**
     * @brief 完成加载，准备关闭
     * @param delay 延迟关闭时间（毫秒）
     */
    void finish(int delay = 500);

signals:
    /**
     * @brief 启动画面关闭完成信号
     */
    void finished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();
    void applyStyle();

    // UI 组件
    QLabel *m_logoLabel;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QLabel *m_statusLabel;
    QLabel *m_versionLabel;
    QProgressBar *m_progressBar;

    // 布局
    QVBoxLayout *m_mainLayout;

    // 动画
    QGraphicsOpacityEffect *m_opacityEffect;
    QPropertyAnimation *m_fadeAnimation;
};

} // namespace UI
} // namespace GenPreCVSystem

#endif // SPLASHSCREEN_H
