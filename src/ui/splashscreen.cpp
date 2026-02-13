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

    // ========== Logo 区域 ==========
    m_logoLabel = new QLabel(this);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    // 使用文字作为 Logo（可以替换为图片）
    m_logoLabel->setText("🔍");
    m_logoLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 64px;"
        "  background: transparent;"
        "}"
    );
    m_logoLabel->setFixedSize(100, 100);
    m_mainLayout->addWidget(m_logoLabel, 0, Qt::AlignCenter);

    // ========== 标题区域 ==========
    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setText("GenPreCVSystem");
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 28px;"
        "  font-weight: bold;"
        "  color: #ffffff;"
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
        "  color: #888888;"
        "  background: transparent;"
        "}"
    );
    m_mainLayout->addWidget(m_subtitleLabel);

    // ========== 弹性空间 ==========
    m_mainLayout->addStretch();

    // ========== 状态文本 ==========
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setText("正在初始化...");
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 11px;"
        "  color: #aaaaaa;"
        "  background: transparent;"
        "}"
    );
    m_mainLayout->addWidget(m_statusLabel);

    // ========== 进度条 ==========
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(4);
    m_mainLayout->addWidget(m_progressBar);

    // ========== 版本信息 ==========
    m_versionLabel = new QLabel(this);
    m_versionLabel->setAlignment(Qt::AlignRight);
    m_versionLabel->setText("Version 1.0.0");
    m_versionLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 10px;"
        "  color: #666666;"
        "  background: transparent;"
        "}"
    );
    m_mainLayout->addWidget(m_versionLabel);
}

void SplashScreen::applyStyle()
{
    // 进度条样式
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #2d2d30;"
        "  border: none;"
        "  border-radius: 2px;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #0078d4, stop:1 #00bcf2);"
        "  border-radius: 2px;"
        "}"
    );
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), 12, 12);

    // 深色背景
    painter.fillPath(path, QColor("#1e1e1e"));

    // 边框
    painter.setPen(QPen(QColor("#3e3e42"), 1));
    painter.drawPath(path);
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
