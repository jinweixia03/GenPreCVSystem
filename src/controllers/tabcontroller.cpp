#include "tabcontroller.h"
#include <QFileInfo>
#include <QMessageBox>

namespace GenPreCVSystem {
namespace Controllers {

TabController::TabController(QTabWidget *tabWidget, QObject *parent)
    : QObject(parent)
    , m_tabWidget(tabWidget)
    , m_currentUndoStack(nullptr)
{
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &TabController::onTabCloseRequested);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &TabController::onCurrentTabChanged);
}

TabController::~TabController()
{
    // 清理撤销栈
    qDeleteAll(m_tabData);
}

bool TabController::loadImage(const QString &filePath)
{
    QPixmap pixmap(filePath);

    if (pixmap.isNull()) {
        emit imageLoaded(filePath, QPixmap());
        return false;
    }

    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    // 检查是否已在标签页中打开
    int existingIndex = findTabIndex(filePath);
    if (existingIndex >= 0) {
        // 已存在，切换到该标签页
        m_tabWidget->setCurrentIndex(existingIndex);
        emit imageLoaded(filePath, pixmap);
        return true;
    }

    // 创建新的标签页
    Views::ImageView *imageView = new Views::ImageView(m_tabWidget);
    imageView->setPixmap(pixmap);

    // 添加标签页
    int index = m_tabWidget->addTab(imageView, fileName);
    m_tabWidget->setCurrentIndex(index);

    // 初始化标签页数据
    Models::TabData tabData(filePath, pixmap);
    tabData.undoStack = QStack<QPixmap>();
    tabData.redoStack = QStack<QPixmap>();
    m_tabData[index] = tabData;

    // 创建撤销栈
    m_tabData[index].undoStack = QStack<QPixmap>();
    m_tabData[index].redoStack = QStack<QPixmap>();

    // 更新当前引用
    updateCurrentTabRef();

    emit imageLoaded(filePath, pixmap);

    return true;
}

void TabController::closeCurrentTab()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0) {
        closeTab(currentIndex);
    }
}

void TabController::closeTab(int index)
{
    if (index < 0 || index >= m_tabWidget->count()) {
        return;
    }

    // 移除标签页数据
    m_tabData.remove(index);

    // 移除标签页
    m_tabWidget->removeTab(index);

    // 重新映射标签页数据索引
    QHash<int, Models::TabData> newTabData;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabData.contains(i)) {
            newTabData[i] = m_tabData[i];
        }
    }
    m_tabData = newTabData;

    emit tabCloseRequested(index);
}

void TabController::switchToTab(int index)
{
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

int TabController::currentTabIndex() const
{
    return m_tabWidget->currentIndex();
}

int TabController::tabCount() const
{
    return m_tabWidget->count();
}

Views::ImageView* TabController::currentImageView() const
{
    if (m_tabWidget->currentIndex() >= 0) {
        return qobject_cast<Views::ImageView*>(m_tabWidget->currentWidget());
    }
    return nullptr;
}

QString TabController::currentImagePath() const
{
    return m_currentImagePath;
}

QPixmap TabController::currentPixmap() const
{
    return m_currentPixmap;
}

void TabController::updateCurrentPixmap(const QPixmap &pixmap)
{
    m_currentPixmap = pixmap;

    // 更新标签页数据
    int index = m_tabWidget->currentIndex();
    if (index >= 0 && m_tabData.contains(index)) {
        m_tabData[index].pixmap = pixmap;
    }

    // 更新显示
    Views::ImageView *view = currentImageView();
    if (view) {
        view->setPixmap(pixmap);
    }
}

Models::UndoStack* TabController::currentUndoStack()
{
    // 这里需要返回真正的UndoStack对象
    // 目前先返回nullptr，后续需要集成UndoStack模型
    return m_currentUndoStack;
}

int TabController::findTabIndex(const QString &filePath)
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabData.contains(i) && m_tabData[i].imagePath == filePath) {
            return i;
        }
    }
    return -1;
}

void TabController::saveState()
{
    int index = m_tabWidget->currentIndex();
    if (index >= 0 && m_tabData.contains(index)) {
        if (!m_currentPixmap.isNull()) {
            m_tabData[index].undoStack.push(m_currentPixmap);
            // 限制撤销栈大小
            if (m_tabData[index].undoStack.size() > 50) {
                m_tabData[index].undoStack.remove(0);
            }
            // 清空重做栈
            m_tabData[index].redoStack.clear();
        }
    }
}

void TabController::onTabCloseRequested(int index)
{
    closeTab(index);
}

void TabController::onCurrentTabChanged(int index)
{
    updateCurrentTabRef();
    emit currentTabChanged(index);
}

void TabController::updateCurrentTabRef()
{
    int index = m_tabWidget->currentIndex();
    if (index >= 0 && m_tabData.contains(index)) {
        const Models::TabData &tabData = m_tabData[index];
        m_currentImagePath = tabData.imagePath;
        m_currentPixmap = tabData.pixmap;
    } else {
        m_currentImagePath.clear();
        m_currentPixmap = QPixmap();
    }
}

QString TabController::getTabTitle(int index) const
{
    if (index >= 0 && index < m_tabWidget->count()) {
        return m_tabWidget->tabText(index);
    }
    return QString();
}

} // namespace Controllers
} // namespace GenPreCVSystem
