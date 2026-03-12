/**
 * @file splashscreen.cpp
 * @brief 启动画面实现
 *
 * 在应用程序启动时显示的启动画面，包含：
 * - 应用程序 Logo
 * - 版本信息
 * - 加载进度条
 * - 状态文本
 */

#include "splashscreen.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>

namespace GenPreCVSystem {
namespace UI {

SplashScreen::SplashScreen(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_opacityEffect(nullptr)
    , m_fadeAnimation(nullptr)
{
    setupUI();
    applyStyle();

    // 设置窗口透明（圆角效果）
    setAttribute(Qt::WA_TranslucentBackground);

    // 设置窗口大小
    setFixedSize(480, 320);

    // 居中显示
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.center() - rect().center());
    }
}

SplashScreen::~SplashScreen()
{
}

void SplashScreen::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(30, 40, 30, 30);

    // ========== Logo 区域（带装饰图标）==========
    QHBoxLayout *logoLayout = new QHBoxLayout();
    logoLayout->setSpacing(8);

    // 左侧装饰图标
    QLabel *leftIcon = new QLabel(this);
    leftIcon->setText("◇");
    leftIcon->setStyleSheet("QLabel { font-size: 24px; color: #0066cc; background: transparent; }");
    leftIcon->setFixedSize(30, 100);
    leftIcon->setAlignment(Qt::AlignCenter);
    logoLayout->addWidget(leftIcon);

    // 主 Logo
    m_logoLabel = new QLabel(this);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    // 使用蓝色方块图标作为 Logo
    m_logoLabel->setText("▣");
    m_logoLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 72px;"
        "  color: #0066cc;"
        "  background: transparent;"
        "}"
    );
    m_logoLabel->setFixedSize(100, 100);
    logoLayout->addWidget(m_logoLabel);

    // 右侧装饰图标
    QLabel *rightIcon = new QLabel(this);
    rightIcon->setText("◇");
    rightIcon->setStyleSheet("QLabel { font-size: 24px; color: #0066cc; background: transparent; }");
    rightIcon->setFixedSize(30, 100);
    rightIcon->setAlignment(Qt::AlignCenter);
    logoLayout->addWidget(rightIcon);

    m_mainLayout->addLayout(logoLayout);

    // ========== 标题区域 ==========
    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setText("GenPreCVSystem");
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 28px;"
        "  font-weight: bold;"
        "  color: #000000;"
        "  background: transparent;"
        "}"
    );
    m_mainLayout->addWidget(m_titleLabel);

    // ========== 副标题 ==========
    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setText("Computer Vision Preprocessing System");
    m_subtitleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 12px;"
        "  color: #333333;"
        "  background: transparent;"
        "}"
    );
    m_mainLayout->addWidget(m_subtitleLabel);

    // ========== 弹性空间 ==========
    m_mainLayout->addStretch();

    // ========== 状态文本（带加载图标）==========
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(8);

    // 加载动画图标
    QLabel *loadingIcon = new QLabel(this);
    loadingIcon->setText("⟳");
    loadingIcon->setStyleSheet("QLabel { font-size: 14px; color: #0066cc; background: transparent; }");
    loadingIcon->setFixedSize(20, 20);
    loadingIcon->setAlignment(Qt::AlignCenter);
    statusLayout->addWidget(loadingIcon);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setText("正在初始化...");
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 11px;"
        "  color: #666666;"
        "  background: transparent;"
        "}"
    );
    statusLayout->addWidget(m_statusLabel);

    // 右侧装饰点
    QLabel *dotIcon = new QLabel(this);
    dotIcon->setText("●");
    dotIcon->setStyleSheet("QLabel { font-size: 6px; color: #0066cc; background: transparent; }");
    dotIcon->setFixedSize(20, 20);
    dotIcon->setAlignment(Qt::AlignCenter);
    statusLayout->addWidget(dotIcon);

    statusLayout->setStretch(0, 1);
    statusLayout->setStretch(1, 10);
    statusLayout->setStretch(2, 1);
    m_mainLayout->addLayout(statusLayout);

    // ========== 进度条 ==========
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_mainLayout->addWidget(m_progressBar);

    // ========== 版本信息 ==========
    m_versionLabel = new QLabel(this);
    m_versionLabel->setAlignment(Qt::AlignRight);
    m_versionLabel->setText("Version 1.0.0");
    m_versionLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 10px;"
        "  color: #999999;"
        "  background: transparent;"
        "}"
    );
    m_mainLayout->addWidget(m_versionLabel);
}

void SplashScreen::applyStyle()
{
    // 进度条样式 - 白色背景，蓝色进度
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #e0e0e0;"
        "  border: 1px solid #0066cc;"
        "  border-radius: 0px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #0066cc;"
        "  border-radius: 0px;"
        "}"
    );
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    // 移除抗锯齿以保持方正边缘
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 绘制方正背景（无圆角）- 白色背景
    painter.fillRect(rect(), QColor("#ffffff"));

    // 蓝色边框
    painter.setPen(QPen(QColor("#0066cc"), 2));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));

    // 绘制角落装饰图案
    painter.setPen(QPen(QColor("#0066cc"), 1));

    // 左上角装饰
    int cornerSize = 20;
    int margin = 8;
    painter.drawLine(margin, margin, margin + cornerSize, margin);
    painter.drawLine(margin, margin, margin, margin + cornerSize);

    // 右上角装饰
    painter.drawLine(width() - margin - cornerSize, margin, width() - margin, margin);
    painter.drawLine(width() - margin, margin, width() - margin, margin + cornerSize);

    // 左下角装饰
    painter.drawLine(margin, height() - margin, margin + cornerSize, height() - margin);
    painter.drawLine(margin, height() - margin - cornerSize, margin, height() - margin);

    // 右下角装饰
    painter.drawLine(width() - margin - cornerSize, height() - margin, width() - margin, height() - margin);
    painter.drawLine(width() - margin, height() - margin - cornerSize, width() - margin, height() - margin);

    // 绘制中间分隔线
    painter.setPen(QPen(QColor("#e0e0e0"), 1, Qt::DashLine));
    int lineY = 180;
    painter.drawLine(50, lineY, width() - 50, lineY);
}

void SplashScreen::setProgress(int value)
{
    m_progressBar->setValue(qBound(0, value, 100));
}

void SplashScreen::setStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

void SplashScreen::setVersion(const QString &version)
{
    m_versionLabel->setText(QString("Version %1").arg(version));
}

void SplashScreen::finish(int delay)
{
    // 确保进度条满
    setProgress(100);
    setStatus("准备就绪");

    // 延迟后淡出关闭
    QTimer::singleShot(delay, this, [this]() {
        // 创建淡出动画
        m_opacityEffect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(m_opacityEffect);

        m_fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
        m_fadeAnimation->setDuration(300);
        m_fadeAnimation->setStartValue(1.0);
        m_fadeAnimation->setEndValue(0.0);

        connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
            hide();
            emit finished();
            deleteLater();
        });

        m_fadeAnimation->start();
    });
}

} // namespace UI
} // namespace GenPreCVSystem
