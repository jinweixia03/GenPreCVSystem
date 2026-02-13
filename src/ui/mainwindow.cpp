#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../controllers/taskcontroller.h"
#include "../controllers/parameterpanelfactory.h"
#include "../utils/recentfilesmanager.h"
#include "../utils/appsettings.h"
#include "../views/settingsdialog.h"
#include "../views/batchprocessdialog.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QDateTime>
#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QClipboard>
#include <QTransform>
#include <QApplication>
#include <QInputDialog>
#include <QPainter>
#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QComboBox>
#include <QStack>
#include <QMenu>
#include <QAction>
#include <QMimeData>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QDesktopServices>
#include <QUrl>
#include <QBuffer>
#include <QToolBar>
#include <QActionGroup>
#include <QScrollArea>
#include <QSlider>
#include <cmath>

// ==================== ImageView 实现 ====================

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_pixmapItem(nullptr)
    , m_scaleFactor(1.0)
    , m_dragging(false)
{
    // 创建图形场景
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // 设置外观样式（使用透明背景，让主题系统控制）
    setStyleSheet(
        "ImageView { background-color: transparent; border: none; }"
    );

    // 设置渲染选项（抗锯齿、平滑）
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 设置拖拽模式为滚动拖拽
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    // 隐藏滚动条（使用鼠标拖拽平移）
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 设置缓存背景
    setCacheMode(QGraphicsView::CacheBackground);
}

/**
 * @brief 设置要显示的图片
 * @param pixmap 图片数据
 */
void ImageView::setPixmap(const QPixmap &pixmap)
{
    // 清空场景
    m_scene->clear();
    m_pixmapItem = nullptr;

    if (pixmap.isNull()) {
        return;
    }

    // 创建图片项并添加到场景
    m_pixmapItem = m_scene->addPixmap(pixmap);
    m_scene->setSceneRect(pixmap.rect());

    // 自适应窗口大小
    fitToWindow();
}

/**
 * @brief 清空显示的图片
 */
void ImageView::clearImage()
{
    m_scene->clear();
    m_pixmapItem = nullptr;
    m_scaleFactor = 1.0;
    resetTransform();
}

/**
 * @brief 缩放图片
 * @param factor 缩放因子（>1 放大，<1 缩小）
 */
void ImageView::scaleImage(double factor)
{
    if (!m_pixmapItem) {
        return;
    }

    // 限制缩放范围（0.1x ~ 10x）
    double newScale = m_scaleFactor * factor;
    if (newScale < 0.1 || newScale > 10.0) {
        return;
    }

    scale(factor, factor);
    m_scaleFactor = newScale;
}

/**
 * @brief 自适应窗口大小显示图片
 */
void ImageView::fitToWindow()
{
    if (!m_pixmapItem) {
        return;
    }

    // 获取视图和图片尺寸
    QSize viewSize = viewport()->size();
    QSize imageSize = m_pixmapItem->pixmap().size();

    if (imageSize.isEmpty()) {
        return;
    }

    // 计算缩放比例
    double scaleX = static_cast<double>(viewSize.width()) / imageSize.width();
    double scaleY = static_cast<double>(viewSize.height()) / imageSize.height();
    double scaleFactor = qMin(scaleX, scaleY) * 0.95;  // 留5%边距

    // 重置变换并应用新缩放
    resetTransform();
    QGraphicsView::scale(scaleFactor, scaleFactor);
    m_scaleFactor = scaleFactor;

    // 居中显示
    centerOn(m_pixmapItem);
}

/**
 * @brief 缩放至实际大小
 */
void ImageView::actualSize()
{
    if (!m_pixmapItem) {
        return;
    }

    resetTransform();
    m_scaleFactor = 1.0;
    centerOn(m_pixmapItem);
}

/**
 * @brief 获取当前显示的图片
 */
QPixmap ImageView::pixmap() const
{
    if (m_pixmapItem) {
        return m_pixmapItem->pixmap();
    }
    return QPixmap();
}

/**
 * @brief 鼠标滚轮事件，用于缩放图片
 */
void ImageView::wheelEvent(QWheelEvent *event)
{
    if (!m_pixmapItem) {
        return;
    }

    // 获取滚轮滚动的角度
    QPoint angleDelta = event->angleDelta();
    double delta = angleDelta.y();

    // 计算缩放因子（每次滚轮滚动缩放10%）
    double factor = (delta > 0) ? 1.1 : 0.9;

    // 缩放图片
    scaleImage(factor);
}

/**
 * @brief 鼠标按下事件，用于开始拖拽
 */
void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);
}

/**
 * @brief 鼠标移动事件，用于拖拽图片
 */
void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && m_pixmapItem) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();

        // 平移视图
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }
    QGraphicsView::mouseMoveEvent(event);
}

/**
 * @brief 鼠标释放事件，用于结束拖拽
 */
void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

/**
 * @brief 调整大小事件，用于重新适应窗口
 */
void ImageView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    // 窗口大小改变时重新适应窗口
    if (m_pixmapItem) {
        fitToWindow();
    }
}

// ==================== FileTreeView 实现 ====================

/**
 * @brief 构造函数
 */
FileTreeView::FileTreeView(QWidget *parent)
    : QTreeView(parent)
{
    // 启用拖放
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
}

/**
 * @brief 拖放进入事件
 */
void FileTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

/**
 * @brief 拖放移动事件
 */
void FileTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragMoveEvent(event);
    }
}

/**
 * @brief 拖放释放事件
 */
void FileTreeView::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    // 处理文件URL
    if (mimeData->hasUrls()) {
        QStringList imageFiles;
        QString folderPath;
        QList<QUrl> urls = mimeData->urls();

        for (const QUrl &url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);

                if (fileInfo.isDir()) {
                    // 如果是文件夹，记录文件夹路径
                    folderPath = filePath;
                    break;  // 只处理第一个文件夹
                }

                // 检查是否是图片文件
                if (!QPixmap(filePath).isNull()) {
                    imageFiles.append(filePath);
                }
            }
        }

        // 优先处理文件夹（打开文件夹）
        if (!folderPath.isEmpty()) {
            emit folderDropped(folderPath);
            event->acceptProposedAction();
            return;
        }

        // 处理图片文件
        if (!imageFiles.isEmpty()) {
            emit imagesDropped(imageFiles);
            event->acceptProposedAction();
            return;
        }
    }

    // 处理图片数据
    if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");

            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            QString sourceName = QString("dropped_%1.png").arg(timestamp);

            emit imageDataDropped(byteArray, sourceName);
            event->acceptProposedAction();
            return;
        }
    }

    QTreeView::dropEvent(event);
}

// ==================== ImageFileFilterProxyModel 实现 ====================

/**
 * @brief 构造函数
 * @param parent 父对象
 */
ImageFileFilterProxyModel::ImageFileFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

/**
 * @brief 过滤函数，判断是否接受该行
 * @param sourceRow 源模型行号
 * @param sourceParent 源模型父索引
 * @return true 接受该行，false 过滤掉该行
 */
bool ImageFileFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QFileSystemModel *fileModel = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fileModel) {
        return true;
    }

    QModelIndex index = fileModel->index(sourceRow, 0, sourceParent);
    if (!index.isValid()) {
        return false;
    }

    // 始终显示目录
    if (fileModel->isDir(index)) {
        return true;
    }

    // 对于文件，只显示图片格式
    QString fileName = fileModel->fileName(index);
    QString suffix = QFileInfo(fileName).suffix().toLower();

    QStringList imageSuffixes = {
        "png", "jpg", "jpeg", "bmp", "gif", "tiff", "tif", "webp",
        "ico", "svg", "ppm", "pgm", "pbm", "pnm", "xbm", "xpm",
        "jp2", "j2k", "jpf", "jpx", "heic", "heif", "avif", "dng"
    };

    return imageSuffixes.contains(suffix);
}

// ==================== MainWindow 实现 ====================

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fileModel(nullptr)
    , proxyModel(nullptr)
    , tabWidget(nullptr)
    , dockFileBrowser(nullptr)
    , dockParameters(nullptr)
    , dockLogOutput(nullptr)
    , treeViewFiles(nullptr)
    , textEditLog(nullptr)
    , taskActionGroup(nullptr)
    , m_currentTask(CVTask::ImageClassification)
    , m_recentFilesManager(nullptr)
    , m_batchProcessDialog(nullptr)
{
    ui->setupUi(this);

    // 必须先创建停靠窗口（包括 paramScrollArea），然后才能初始化任务菜单
    setupImageViewer();
    setupDockWidgets();
    setupTaskMenus();

    // 初始化最近文件管理器
    m_recentFilesManager = new GenPreCVSystem::Utils::RecentFilesManager(this);
    m_recentFilesManager->setupMenu(ui->menuFile, ui->actionExit);
    connect(m_recentFilesManager, &GenPreCVSystem::Utils::RecentFilesManager::recentFileTriggered,
            this, &MainWindow::onRecentFileTriggered);

    logMessage("应用程序已启动");
    logMessage("提示：使用\"文件\"菜单打开图片，或双击文件浏览器中的图片");
    logMessage("提示：切换到\"任务\"→\"目标检测\"或\"语义分割\"可使用 YOLO 推理功能");
}

/**
 * @brief 析构函数
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief 创建主工作区图片展示器（标签页模式）
 */
void MainWindow::setupImageViewer()
{
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #0066cc; background: #ffffff; }"
        "QTabBar::tab { background: #e0e0e0; color: #000000; padding: 6px 12px; border: 1px solid #c0c0c0; }"
        "QTabBar::tab:selected { background: #0066cc; color: #ffffff; }"
        "QTabBar::tab:hover { background: #f0f0f0; }"
        "QTabBar::close-button { image: url(:/icons/close.png); subcontrol-position: right; }"
        "QTabBar::close-button:hover { background-color: #cc3300; }"
    );

    // 连接标签页关闭和切换信号
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    setCentralWidget(tabWidget);
}

/**
 * @brief 加载并显示图片（标签页模式）
 * @param filePath 图片文件路径
 * @return true 成功加载，false 加载失败
 */
bool MainWindow::loadImage(const QString &filePath)
{
    QPixmap pixmap(filePath);

    if (pixmap.isNull()) {
        logMessage(QString("加载失败: %1").arg(filePath));
        return false;
    }

    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    // 检查是否已在标签页中打开
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (m_tabData.contains(i) && m_tabData[i].imagePath == filePath) {
            // 已存在，切换到该标签页
            tabWidget->setCurrentIndex(i);
            logMessage(QString("切换到已打开的图片: %1").arg(fileName));
            return true;
        }
    }

    // 创建新的标签页
    ImageView *imageView = new ImageView(tabWidget);
    imageView->setPixmap(pixmap);

    // 添加标签页
    int index = tabWidget->addTab(imageView, fileName);
    tabWidget->setCurrentIndex(index);

    // 初始化标签页数据
    TabData tabData;
    tabData.imagePath = filePath;
    tabData.pixmap = pixmap;
    m_tabData[index] = tabData;

    // 更新当前引用
    updateCurrentTabRef();

    // 添加到最近文件列表
    if (m_recentFilesManager) {
        m_recentFilesManager->addFile(filePath);
    }

    logMessage(QString("已加载图片: %1 [%2x%3]")
        .arg(fileName)
        .arg(pixmap.width())
        .arg(pixmap.height()));

    return true;
}

/**
 * @brief 关闭当前图片
 */
void MainWindow::closeImage()
{
    int currentIndex = tabWidget->currentIndex();
    if (currentIndex >= 0) {
        onTabCloseRequested(currentIndex);
    }
}

/**
 * @brief 创建并设置所有停靠窗口
 */
void MainWindow::setupDockWidgets()
{
    // ========== 创建左侧文件浏览器 ==========
    dockFileBrowser = new QDockWidget("📁 文件浏览器", this);
    dockFileBrowser->setMinimumWidth(200);
    dockFileBrowser->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    // 创建容器widget以包含工具栏和树视图
    QWidget *browserContainer = new QWidget(this);
    QVBoxLayout *browserLayout = new QVBoxLayout(browserContainer);
    browserLayout->setContentsMargins(0, 0, 0, 0);
    browserLayout->setSpacing(0);

    // 创建导航工具栏
    QWidget *navBar = new QWidget(browserContainer);
    navBar->setStyleSheet("background-color: #f5f5f5; border-bottom: 2px solid #0066cc;");
    QHBoxLayout *navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(5, 5, 5, 5);

    // 文件夹图标装饰
    QLabel *folderIcon = new QLabel("📁", navBar);
    folderIcon->setStyleSheet("font-size: 14px; background: transparent;");
    folderIcon->setFixedSize(20, 24);
    folderIcon->setAlignment(Qt::AlignCenter);
    navLayout->addWidget(folderIcon);

    // 向上按钮
    QPushButton *btnUp = new QPushButton("⬆", navBar);
    btnUp->setFixedSize(24, 24);
    btnUp->setToolTip("向上一级");
    btnUp->setStyleSheet(
        "QPushButton { background-color: #0066cc; color: #ffffff; border: none; }"
        "QPushButton:hover { background-color: #0077dd; }"
        "QPushButton:pressed { background-color: #0055aa; }"
    );
    connect(btnUp, &QPushButton::clicked, this, &MainWindow::navigateUp);

    // 刷新按钮
    QPushButton *btnRefresh = new QPushButton("⟳", navBar);
    btnRefresh->setFixedSize(24, 24);
    btnRefresh->setToolTip("刷新");
    btnRefresh->setStyleSheet(
        "QPushButton { background-color: #0066cc; color: #ffffff; border: none; }"
        "QPushButton:hover { background-color: #0077dd; }"
        "QPushButton:pressed { background-color: #0055aa; }"
    );
    connect(btnRefresh, &QPushButton::clicked, this, [this]() {
        if (fileModel) {
            fileModel->setRootPath(fileModel->rootPath());
            logMessage("文件列表已刷新");
        }
    });

    // 当前路径显示
    labelCurrentPath = new QLabel("/", navBar);
    labelCurrentPath->setStyleSheet("color: #000000; padding: 2px;");
    labelCurrentPath->setWordWrap(true);

    navLayout->addWidget(btnUp);
    navLayout->addWidget(btnRefresh);
    navLayout->addWidget(labelCurrentPath, 1);

    browserLayout->addWidget(navBar);

    // 创建文件树视图
    treeViewFiles = new FileTreeView(this);
    treeViewFiles->setStyleSheet(
        "QTreeView { background-color: #ffffff; color: #000000; border: none; }"
        "QTreeView::item { padding: 3px; }"
        "QTreeView::item:hover { background-color: #e0e0e0; }"
        "QTreeView::item:selected { background-color: #0066cc; color: #ffffff; }"
    );
    treeViewFiles->setHeaderHidden(true);
    treeViewFiles->setFrameShape(QFrame::NoFrame);

    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(QDir::rootPath());

    proxyModel = new ImageFileFilterProxyModel(this);
    proxyModel->setSourceModel(fileModel);

    treeViewFiles->setModel(proxyModel);
    treeViewFiles->hideColumn(1);
    treeViewFiles->hideColumn(2);
    treeViewFiles->hideColumn(3);

    connect(treeViewFiles, &QTreeView::doubleClicked,
            this, &MainWindow::onFileTreeDoubleClicked);

    // 设置自定义上下文菜单策略
    treeViewFiles->setContextMenuPolicy(Qt::CustomContextMenu);

    // 连接上下文菜单请求信号
    connect(treeViewFiles, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onFileTreeContextMenuRequested);

    // 连接拖放信号
    connect(treeViewFiles, &FileTreeView::imagesDropped,
            this, &MainWindow::onImagesDropped);
    connect(treeViewFiles, &FileTreeView::imageDataDropped,
            this, &MainWindow::onImageDataDropped);
    connect(treeViewFiles, &FileTreeView::folderDropped,
            this, &MainWindow::onFolderDropped);

    browserLayout->addWidget(treeViewFiles);

    dockFileBrowser->setWidget(browserContainer);
    addDockWidget(Qt::LeftDockWidgetArea, dockFileBrowser);

    // 同步停靠窗口可见性与菜单选中状态
    connect(dockFileBrowser, &QDockWidget::visibilityChanged,
            ui->actionShowFileBrowser, &QAction::setChecked);

    // ========== 创建右侧参数设置面板 ==========
    dockParameters = new QDockWidget("⚙ 参数设置", this);
    dockParameters->setMinimumWidth(250);
    dockParameters->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    // 创建滚动区域以支持动态内容
    paramScrollArea = new QScrollArea(this);
    paramScrollArea->setStyleSheet(
        "QScrollArea { background-color: #ffffff; border: none; }"
        "QScrollBar:vertical { background-color: #e0e0e0; width: 10px; }"
        "QScrollBar::handle:vertical { background-color: #0066cc; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background-color: #0077dd; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );
    paramScrollArea->setWidgetResizable(true);
    paramScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    paramWidget = new QWidget();
    paramWidget->setStyleSheet("background-color: #ffffff;");
    dockParameters->setWidget(paramScrollArea);
    addDockWidget(Qt::RightDockWidgetArea, dockParameters);

    // 同步停靠窗口可见性与菜单选中状态
    connect(dockParameters, &QDockWidget::visibilityChanged,
            ui->actionShowParameterPanel, &QAction::setChecked);

    // ========== 创建底部日志输出区域 ==========
    dockLogOutput = new QDockWidget("📋 日志输出", this);
    dockLogOutput->setMinimumHeight(150);
    dockLogOutput->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    textEditLog = new QTextEdit(this);
    textEditLog->setStyleSheet(
        "QTextEdit { background-color: #ffffff; color: #000000; border: 1px solid #c0c0c0; "
        "font-family: Consolas, Monaco, monospace; }"
    );
    textEditLog->setFrameShape(QFrame::NoFrame);
    textEditLog->setReadOnly(true);

    dockLogOutput->setWidget(textEditLog);
    addDockWidget(Qt::BottomDockWidgetArea, dockLogOutput);

    // 同步停靠窗口可见性与菜单选中状态
    connect(dockLogOutput, &QDockWidget::visibilityChanged,
            ui->actionShowLogOutput, &QAction::setChecked);
}

/**
 * @brief 设置文件浏览器（已在 setupDockWidgets 中实现）
 */
void MainWindow::setupFileBrowser()
{
    // 已在 setupDockWidgets 中实现
}

/**
 * @brief 设置日志输出区域（已在 setupDockWidgets 中实现）
 */
void MainWindow::setupLogOutput()
{
    // 已在 setupDockWidgets 中实现
}

/**
 * @brief 设置参数面板（已在 setupDockWidgets 中实现）
 */
void MainWindow::setupParameterPanel()
{
    // 已在 setupDockWidgets 中实现
}

/**
 * @brief 向日志输出区域添加消息
 */
void MainWindow::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    textEditLog->append(QString("▸ [%1] %2").arg(timestamp, message));
}

/**
 * @brief 获取支持的图片格式过滤器字符串
 */
QString MainWindow::getImageFileFilter() const
{
    return "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.tif *.webp *.ico *.svg *.ppm *.pgm *.pbm *.pnm *.xbm *.xpm *.jp2 *.j2k *.jpf *.jpx *.heic *.heif *.avif *.dng);;All Files (*)";
}

// ==================== 文件菜单槽函数 ====================

/**
 * @brief 打开图片文件
 */
void MainWindow::on_actionOpenImage_triggered()
{
    QString defaultDir = GenPreCVSystem::Utils::AppSettings::defaultOpenDirectory();
    QString fileName = QFileDialog::getOpenFileName(
        this, "打开图片文件", defaultDir, getImageFileFilter()
    );

    if (!fileName.isEmpty()) {
        if (loadImage(fileName)) {
            QFileInfo fileInfo(fileName);
            QString dirPath = fileInfo.absolutePath();
            QDir dir(dirPath);

            if (dir.exists()) {
                QModelIndex sourceIndex = fileModel->index(dirPath);
                QModelIndex proxyIndex = proxyModel->mapFromSource(sourceIndex);
                treeViewFiles->setRootIndex(proxyIndex);
                m_currentBrowsePath = dirPath;
                labelCurrentPath->setText(dirPath);
                logMessage(QString("已定位到目录: %1").arg(dirPath));
            }
        }
    }
}

/**
 * @brief 打开文件夹
 */
void MainWindow::on_actionOpenFolder_triggered()
{
    QString defaultDir = GenPreCVSystem::Utils::AppSettings::defaultOpenDirectory();
    QString dirPath = QFileDialog::getExistingDirectory(this, "打开图片文件夹", defaultDir);

    if (!dirPath.isEmpty()) {
        QModelIndex sourceIndex = fileModel->index(dirPath);
        QModelIndex proxyIndex = proxyModel->mapFromSource(sourceIndex);
        treeViewFiles->setRootIndex(proxyIndex);
        m_currentBrowsePath = dirPath;
        labelCurrentPath->setText(dirPath);
        logMessage(QString("已打开目录: %1").arg(dirPath));
    }
}

/**
 * @brief 关闭当前图片
 */
void MainWindow::on_actionCloseImage_triggered()
{
    closeImage();
}

/**
 * @brief 保存图片
 */
void MainWindow::on_actionSaveImage_triggered()
{
    if (m_currentImagePath.isEmpty() || m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "没有可保存的图片");
        return;
    }

    // 检查文件是否存在
    QFileInfo fileInfo(m_currentImagePath);

    if (fileInfo.exists()) {
        // 文件存在，显示选项对话框
        QString previewPath = generateCopyFilePath(m_currentImagePath);
        QString previewName = QFileInfo(previewPath).fileName();

        int choice = showSaveOptionsDialog(previewName);

        if (choice == -1) {
            // 用户取消
            return;
        } else if (choice == 1) {
            // 保存副本
            if (m_currentPixmap.save(previewPath)) {
                logMessage(QString("已保存副本: %1").arg(previewPath));
                return;
            } else {
                QMessageBox::warning(this, "错误", "保存副本失败");
                return;
            }
        }
        // choice == 0: 覆盖原文件，继续执行下面的保存代码
    }

    // 直接保存到原路径
    if (m_currentPixmap.save(m_currentImagePath)) {
        logMessage(QString("已保存: %1").arg(m_currentImagePath));
    } else {
        QMessageBox::warning(this, "错误", "保存失败");
    }
}

/**
 * @brief 另存为
 */
void MainWindow::on_actionSaveImageAs_triggered()
{
    if (m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "没有可保存的图片");
        return;
    }

    QString defaultDir = GenPreCVSystem::Utils::AppSettings::defaultExportDirectory();
    QString fileName = QFileDialog::getSaveFileName(
        this, "另存为", defaultDir, getImageFileFilter()
    );

    if (!fileName.isEmpty()) {
        if (m_currentPixmap.save(fileName)) {
            logMessage(QString("已保存: %1").arg(fileName));
            m_currentImagePath = fileName;
            if (m_taskController) {
                m_taskController->setCurrentImagePath(fileName);
            }
        } else {
            QMessageBox::warning(this, "错误", "保存失败");
        }
    }
}

/**
 * @brief 导出图片
 */
void MainWindow::on_actionExport_triggered()
{
    if (m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "没有可导出的图片");
        return;
    }

    QString defaultDir = GenPreCVSystem::Utils::AppSettings::defaultExportDirectory();
    QString defaultName = m_currentImagePath.isEmpty()
        ? QString("%1/exported_%2.png").arg(defaultDir).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
        : QString("%1/%2_exported.png").arg(defaultDir).arg(QFileInfo(m_currentImagePath).baseName());

    QString fileName = QFileDialog::getSaveFileName(
        this, "导出图片", defaultName, getImageFileFilter()
    );

    if (!fileName.isEmpty()) {
        if (m_currentPixmap.save(fileName)) {
            logMessage(QString("图片已导出: %1").arg(fileName));
            QMessageBox::information(this, "成功", "图片导出成功！");
        } else {
            QMessageBox::warning(this, "错误", "图片导出失败！");
        }
    }
}

/**
 * @brief 退出应用
 */
void MainWindow::on_actionExit_triggered()
{
    close();
}

// ==================== 编辑菜单槽函数 ====================

/**
 * @brief 撤销上一步操作
 */
void MainWindow::on_actionUndo_triggered()
{
    if (m_undoStack.isEmpty()) {
        logMessage("没有可撤销的操作");
        return;
    }

    // 将当前状态保存到重做栈
    m_redoStack.push(m_currentPixmap);

    // 从撤销栈弹出上一个状态
    m_currentPixmap = m_undoStack.pop();

    // 更新显示
    currentImageView()->setPixmap(m_currentPixmap);

    // 更新按钮状态
    updateUndoRedoState();

    logMessage(QString("撤销 (剩余步骤: %1)").arg(m_undoStack.size()));
}

/**
 * @brief 重做上一步撤销的操作
 */
void MainWindow::on_actionRedo_triggered()
{
    if (m_redoStack.isEmpty()) {
        logMessage("没有可重做的操作");
        return;
    }

    // 将当前状态保存到撤销栈
    m_undoStack.push(m_currentPixmap);

    // 从重做栈弹出状态
    m_currentPixmap = m_redoStack.pop();

    // 更新显示
    currentImageView()->setPixmap(m_currentPixmap);

    // 更新按钮状态
    updateUndoRedoState();

    logMessage(QString("重做 (剩余步骤: %1)").arg(m_redoStack.size()));
}

/**
 * @brief 复制图片到剪贴板（同时复制图片数据和文件引用）
 */
void MainWindow::on_actionCopyImage_triggered()
{
    // 如果焦点在文本编辑控件上，执行文本复制
    QWidget *focusedWidget = QApplication::focusWidget();
    if (qobject_cast<QTextEdit*>(focusedWidget) ||
        qobject_cast<QLineEdit*>(focusedWidget) ||
        qobject_cast<QPlainTextEdit*>(focusedWidget)) {
        // 使用控件的默认复制功能
        if (QTextEdit *textEdit = qobject_cast<QTextEdit*>(focusedWidget)) {
            textEdit->copy();
            return;
        }
        if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(focusedWidget)) {
            lineEdit->copy();
            return;
        }
        if (QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit*>(focusedWidget)) {
            plainTextEdit->copy();
            return;
        }
    }

    if (m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "没有可复制的图片");
        return;
    }

    // 创建 QMimeData 同时支持两种格式
    QMimeData *mimeData = new QMimeData();

    // 1. 设置图片数据 - 用于在图片编辑器、Word等应用中粘贴图片内容
    mimeData->setImageData(m_currentPixmap.toImage());

    // 2. 如果有文件路径，同时设置文件引用 - 用于在文件管理器中粘贴文件副本
    if (!m_currentImagePath.isEmpty()) {
        QList<QUrl> urls;
        urls << QUrl::fromLocalFile(m_currentImagePath);
        mimeData->setUrls(urls);
        logMessage(QString("已复制图片和文件: %1").arg(QFileInfo(m_currentImagePath).fileName()));
    } else {
        logMessage("已复制图片到剪贴板");
    }

    QApplication::clipboard()->setMimeData(mimeData);
}

/**
 * @brief 从剪贴板粘贴图片（保存到当前目录并显示）
 */
void MainWindow::on_actionPaste_triggered()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData) {
        QMessageBox::warning(this, "提示", "剪贴板为空");
        return;
    }

    QPixmap pixmap;
    QString sourceFileName;

    // 优先从 URL 获取文件（保留原文件名）
    if (mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);

                // 检查是否是图片文件
                if (QPixmap(filePath).isNull()) {
                    continue;
                }

                pixmap = QPixmap(filePath);
                sourceFileName = fileInfo.fileName();
                break;
            }
        }
    }

    // 如果没有从 URL 获取到图片，尝试直接获取图片数据
    if (pixmap.isNull() && mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            pixmap = QPixmap::fromImage(image);
            // 使用时间戳生成文件名
            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            sourceFileName = QString("pasted_%1.png").arg(timestamp);
        }
    }

    if (pixmap.isNull()) {
        QMessageBox::warning(this, "提示", "剪贴板中没有图片");
        return;
    }

    // 确定保存路径
    QDir currentDir(m_currentBrowsePath.isEmpty() ? QDir::homePath() : m_currentBrowsePath);
    QString targetPath = currentDir.absoluteFilePath(sourceFileName);

    // 检查文件是否存在
    if (QFileInfo::exists(targetPath)) {
        QString previewPath = generateCopyFilePath(targetPath);
        QString previewName = QFileInfo(previewPath).fileName();

        int choice = showSaveOptionsDialog(previewName);

        if (choice == -1) {
            return;  // 用户取消
        } else if (choice == 1) {
            targetPath = previewPath;  // 保存副本
        }
        // choice == 0: 覆盖原文件，使用原路径
    }

    // 保存图片
    if (pixmap.save(targetPath)) {
        // 加载为标签页
        loadImage(targetPath);

        // 刷新视图以显示新文件
        fileModel->setRootPath(m_currentBrowsePath);

        logMessage(QString("已粘贴图片: %1").arg(QFileInfo(targetPath).fileName()));
    } else {
        QMessageBox::critical(this, "错误", "保存图片失败");
    }
}

/**
 * @brief 向左旋转90度
 */
void MainWindow::on_actionRotateLeft_triggered()
{
    if (m_currentPixmap.isNull()) {
        return;
    }

    saveState();  // 保存当前状态到撤销栈

    QTransform transform;
    transform.rotate(-90);
    m_currentPixmap = m_currentPixmap.transformed(transform);
    currentImageView()->setPixmap(m_currentPixmap);
    logMessage("向左旋转90°");
}

/**
 * @brief 向右旋转90度
 */
void MainWindow::on_actionRotateRight_triggered()
{
    if (m_currentPixmap.isNull()) {
        return;
    }

    saveState();  // 保存当前状态到撤销栈

    QTransform transform;
    transform.rotate(90);
    m_currentPixmap = m_currentPixmap.transformed(transform);
    currentImageView()->setPixmap(m_currentPixmap);
    logMessage("向右旋转90°");
}

/**
 * @brief 水平翻转
 */
void MainWindow::on_actionFlipHorizontal_triggered()
{
    if (m_currentPixmap.isNull()) {
        return;
    }

    saveState();  // 保存当前状态到撤销栈

    m_currentPixmap = m_currentPixmap.transformed(QTransform().scale(-1, 1));
    currentImageView()->setPixmap(m_currentPixmap);
    logMessage("水平翻转");
}

/**
 * @brief 垂直翻转
 */
void MainWindow::on_actionFlipVertical_triggered()
{
    if (m_currentPixmap.isNull()) {
        return;
    }

    saveState();  // 保存当前状态到撤销栈

    m_currentPixmap = m_currentPixmap.transformed(QTransform().scale(1, -1));
    currentImageView()->setPixmap(m_currentPixmap);
    logMessage("垂直翻转");
}

// ==================== 视图菜单槽函数 ====================

/**
 * @brief 放大
 */
void MainWindow::on_actionZoomIn_triggered()
{
    currentImageView()->scaleImage(1.1);
    logMessage(QString("放大: %1%").arg(qRound(currentImageView()->currentScale() * 100)));
}

/**
 * @brief 缩小
 */
void MainWindow::on_actionZoomOut_triggered()
{
    currentImageView()->scaleImage(0.9);
    logMessage(QString("缩小: %1%").arg(qRound(currentImageView()->currentScale() * 100)));
}

/**
 * @brief 适应窗口
 */
void MainWindow::on_actionFitToWindow_triggered()
{
    currentImageView()->fitToWindow();
    logMessage("适应窗口");
}

/**
 * @brief 实际大小
 */
void MainWindow::on_actionActualSize_triggered()
{
    currentImageView()->actualSize();
    logMessage("实际大小");
}

/**
 * @brief 显示/隐藏文件浏览器
 */
void MainWindow::on_actionShowFileBrowser_triggered(bool checked)
{
    dockFileBrowser->setVisible(checked);
}

/**
 * @brief 显示/隐藏参数面板
 */
void MainWindow::on_actionShowParameterPanel_triggered(bool checked)
{
    dockParameters->setVisible(checked);
}

/**
 * @brief 显示/隐藏日志输出
 */
void MainWindow::on_actionShowLogOutput_triggered(bool checked)
{
    dockLogOutput->setVisible(checked);
}

// ==================== 图像菜单槽函数 ====================

/**
 * @brief 灰度化（可切换）
 */
void MainWindow::on_actionGrayscale_triggered(bool checked)
{
    if (m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "请先打开图片");
        ui->actionGrayscale->setChecked(false);
        return;
    }

    if (checked) {
        // 应用灰度化
        saveState();

        // 保存原始图片（如果还没有保存）
        if (!m_isGrayscale && !m_isInverted) {
            m_originalPixmap = m_currentPixmap;
        }

        QImage image = m_currentPixmap.toImage();
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QRgb pixel = image.pixel(x, y);
                int gray = qGray(pixel);
                image.setPixel(x, y, qRgb(gray, gray, gray));
            }
        }

        m_currentPixmap = QPixmap::fromImage(image);
        currentImageView()->setPixmap(m_currentPixmap);
        m_isGrayscale = true;
        logMessage("已转换为灰度图");
    } else {
        // 取消灰度化 - 恢复原始图片
        if (!m_originalPixmap.isNull()) {
            m_currentPixmap = m_originalPixmap;
            currentImageView()->setPixmap(m_currentPixmap);
            m_isGrayscale = false;
            m_isInverted = false;
            ui->actionInvert->setChecked(false);
            logMessage("已恢复原始图片");
        }
    }
}

/**
 * @brief 反色（可切换）
 */
void MainWindow::on_actionInvert_triggered(bool checked)
{
    if (m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "请先打开图片");
        ui->actionInvert->setChecked(false);
        return;
    }

    if (checked) {
        // 应用反色
        saveState();

        // 保存原始图片（如果还没有保存）
        if (!m_isGrayscale && !m_isInverted) {
            m_originalPixmap = m_currentPixmap;
        }

        QImage image = m_currentPixmap.toImage();
        image.invertPixels();

        m_currentPixmap = QPixmap::fromImage(image);
        currentImageView()->setPixmap(m_currentPixmap);
        m_isInverted = true;
        logMessage("已反色");
    } else {
        // 取消反色 - 恢复原始图片
        if (!m_originalPixmap.isNull()) {
            m_currentPixmap = m_originalPixmap;
            currentImageView()->setPixmap(m_currentPixmap);
            m_isGrayscale = false;
            m_isInverted = false;
            ui->actionGrayscale->setChecked(false);
            logMessage("已恢复原始图片");
        }
    }
}

/**
 * @brief 模糊
 */
void MainWindow::on_actionBlur_triggered()
{
    if (m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "请先打开图片");
        return;
    }

    bool ok;
    int radius = QInputDialog::getInt(this, "模糊处理", "模糊半径:", 3, 1, 20, 1, &ok);

    if (ok) {
        saveState();  // 保存当前状态到撤销栈

        QImage image = m_currentPixmap.toImage();
        QImage blurred = gaussianBlur(image, radius);
        m_currentPixmap = QPixmap::fromImage(blurred);
        currentImageView()->setPixmap(m_currentPixmap);
        logMessage(QString("模糊处理 (半径=%1)").arg(radius));
    }
}

/**
 * @brief 锐化
 */
void MainWindow::on_actionSharpen_triggered()
{
    if (m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "请先打开图片");
        return;
    }

    bool ok;
    double strength = QInputDialog::getDouble(this, "锐化处理", "锐化强度:", 1.0, 0.1, 5.0, 1, &ok);

    if (ok) {
        saveState();  // 保存当前状态到撤销栈

        QImage image = m_currentPixmap.toImage();
        QImage sharpened = sharpenImage(image, strength);
        m_currentPixmap = QPixmap::fromImage(sharpened);
        currentImageView()->setPixmap(m_currentPixmap);
        logMessage(QString("锐化处理 (强度=%1)").arg(strength));
    }
}

/**
 * @brief 二值化
 */
void MainWindow::on_actionThreshold_triggered()
{
    if (m_currentPixmap.isNull()) {
        QMessageBox::warning(this, "提示", "请先打开图片");
        return;
    }

    bool ok;
    int threshold = QInputDialog::getInt(this, "二值化", "阈值:", 128, 0, 255, 1, &ok);

    if (ok) {
        saveState();  // 保存当前状态到撤销栈

        QImage image = m_currentPixmap.toImage().convertToFormat(QImage::Format_Grayscale8);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                uint8_t pixel = image.bits()[y * image.bytesPerLine() + x];
                image.bits()[y * image.bytesPerLine() + x] = (pixel >= threshold) ? 255 : 0;
            }
        }

        m_currentPixmap = QPixmap::fromImage(image);
        currentImageView()->setPixmap(m_currentPixmap);
        logMessage(QString("二值化 (阈值=%1)").arg(threshold));
    }
}

// ==================== 任务菜单槽函数 ====================

/**
 * @brief 图像分类
 */
void MainWindow::on_actionTaskImageClassification_triggered()
{
    switchTask(CVTask::ImageClassification);
}

/**
 * @brief 目标检测
 */
void MainWindow::on_actionTaskObjectDetection_triggered()
{
    switchTask(CVTask::ObjectDetection);
}

/**
 * @brief 语义分割
 */
void MainWindow::on_actionTaskSemanticSegmentation_triggered()
{
    switchTask(CVTask::SemanticSegmentation);
}

/**
 * @brief 关键点检测
 */
void MainWindow::on_actionTaskKeyPointDetection_triggered()
{
    switchTask(CVTask::KeyPointDetection);
}

/**
 * @brief 图像增强
 */
void MainWindow::on_actionTaskImageEnhancement_triggered()
{
    switchTask(CVTask::ImageEnhancement);
}

/**
 * @brief 图像去噪
 */
void MainWindow::on_actionTaskImageDenoising_triggered()
{
    switchTask(CVTask::ImageDenoising);
}

/**
 * @brief 边缘检测
 */
void MainWindow::on_actionTaskEdgeDetection_triggered()
{
    switchTask(CVTask::EdgeDetection);
}

// ==================== 工具菜单槽函数 ====================

/**
 * @brief 设置
 */
void MainWindow::on_actionSettings_triggered()
{
    GenPreCVSystem::Views::SettingsDialog dialog(this);
    dialog.exec();
}

/**
 * @brief 批量处理
 */
void MainWindow::on_actionBatchProcess_triggered()
{
    if (!m_taskController || !m_taskController->yoloService()) {
        QMessageBox::warning(this, tr("提示"), tr("请先启动 YOLO 服务"));
        return;
    }

    if (!m_batchProcessDialog) {
        m_batchProcessDialog = new GenPreCVSystem::Views::BatchProcessDialog(this);
        // 当对话框关闭时，恢复结果显示功能
        connect(m_batchProcessDialog, &QDialog::finished, this, [this]() {
            if (m_taskController) {
                m_taskController->setShowResultDialog(true);
            }
        });
    }

    // 禁用结果显示（批量处理时不需要弹出结果对话框）
    m_taskController->setShowResultDialog(false);

    // 设置 YOLO 服务（对话框会自动处理模型和任务选择）
    m_batchProcessDialog->setYOLOService(m_taskController->yoloService());

    m_batchProcessDialog->show();
    m_batchProcessDialog->raise();
    m_batchProcessDialog->activateWindow();
}

/**
 * @brief 最近文件被点击
 */
void MainWindow::onRecentFileTriggered(const QString &filePath)
{
    if (loadImage(filePath)) {
        logMessage(QString("从最近文件打开: %1").arg(filePath));
    }
}

// ==================== 帮助菜单槽函数 ====================

/**
 * @brief 使用文档
 */
void MainWindow::on_actionDocumentation_triggered()
{
    QMessageBox::information(this, "使用文档",
        "<h2>GenPreCVSystem 使用文档</h2>"
        "<p>计算机视觉预处理系统 - 支持图像分类、目标检测、语义分割、姿态检测等任务</p>"
        "<hr>"
        "<h3>📁 文件操作</h3>"
        "<ul>"
        "<li><b>打开图片</b> (Ctrl+O) - 打开单个图片文件</li>"
        "<li><b>打开文件夹</b> (Ctrl+D) - 浏览图片目录</li>"
        "<li><b>关闭图片</b> (Ctrl+W) - 关闭当前标签页</li>"
        "<li><b>保存</b> (Ctrl+S) - 保存当前图片</li>"
        "<li><b>另存为</b> (Ctrl+Shift+S) - 保存到新文件</li>"
        "<li><b>导出</b> (Ctrl+Shift+E) - 导出处理结果</li>"
        "</ul>"
        "<h3>🔍 视图操作</h3>"
        "<ul>"
        "<li><b>滚轮</b> - 缩放图片</li>"
        "<li><b>左键拖拽</b> - 平移图片</li>"
        "<li><b>放大</b> (Ctrl++) - 放大显示</li>"
        "<li><b>缩小</b> (Ctrl+-) - 缩小显示</li>"
        "<li><b>适应窗口</b> (Ctrl+F) - 自适应显示</li>"
        "<li><b>实际大小</b> (Ctrl+1) - 100%显示</li>"
        "</ul>"
        "<h3>🖼 图像处理</h3>"
        "<ul>"
        "<li><b>灰度化</b> - 转换为灰度图</li>"
        "<li><b>反色</b> - 颜色反转</li>"
        "<li><b>模糊</b> - 高斯模糊处理</li>"
        "<li><b>锐化</b> - 边缘锐化</li>"
        "<li><b>二值化</b> - 图像二值化</li>"
        "<li><b>旋转</b> (Ctrl+L/R) - 90°旋转</li>"
        "<li><b>翻转</b> (Ctrl+H) - 水平/垂直翻转</li>"
        "</ul>"
        "<h3>🎯 CV 任务</h3>"
        "<ul>"
        "<li><b>图像分类</b> - 识别图像类别</li>"
        "<li><b>目标检测</b> - 检测并标注目标</li>"
        "<li><b>语义分割</b> - 像素级分割</li>"
        "<li><b>姿态检测</b> - 人体关键点检测</li>"
        "</ul>"
        "<h3>⚡ 批量处理</h3>"
        "<ul>"
        "<li><b>批量处理</b> (Ctrl+Shift+B) - 批量处理文件夹图像</li>"
        "<li>支持导出 YOLO 格式标注</li>"
        "<li>支持 ZIP 压缩包导出</li>"
        "</ul>"
        "<h3>⚙ 设置</h3>"
        "<ul>"
        "<li>配置默认打开/导出目录</li>"
        "<li>设置最近文件数量</li>"
        "<li>配置导出格式 (JSON/CSV)</li>"
        "</ul>"
        "<hr>"
        "<p>提示：选择带 ✓ 的 Python 环境以启用 YOLO 功能</p>");
}

/**
 * @brief 快捷键
 */
void MainWindow::on_actionShortcuts_triggered()
{
    QString shortcuts =
        "<h2>快捷键列表</h2>"
        "<table border='1' cellpadding='5' cellspacing='0'>"
        "<tr><th>功能</th><th>快捷键</th></tr>"
        "<tr><td>打开图片</td><td>Ctrl+O</td></tr>"
        "<tr><td>打开文件夹</td><td>Ctrl+D</td></tr>"
        "<tr><td>关闭图片</td><td>Ctrl+W</td></tr>"
        "<tr><td>保存图片</td><td>Ctrl+S</td></tr>"
        "<tr><td>另存为</td><td>Ctrl+Shift+S</td></tr>"
        "<tr><td>复制图片</td><td>Ctrl+C</td></tr>"
        "<tr><td>粘贴图片</td><td>Ctrl+V</td></tr>"
        "<tr><td>向左旋转</td><td>Ctrl+L</td></tr>"
        "<tr><td>向右旋转</td><td>Ctrl+R</td></tr>"
        "<tr><td>水平翻转</td><td>Ctrl+H</td></tr>"
        "<tr><td>垂直翻转</td><td>Ctrl+Shift+H</td></tr>"
        "<tr><td>放大</td><td>Ctrl++</td></tr>"
        "<tr><td>缩小</td><td>Ctrl+-</td></tr>"
        "<tr><td>适应窗口</td><td>Ctrl+F</td></tr>"
        "<tr><td>实际大小</td><td>Ctrl+1</td></tr>"
        "<tr><td>运行处理</td><td>F5</td></tr>"
        "<tr><td>停止处理</td><td>Shift+F5</td></tr>"
        "<tr><td>设置</td><td>Ctrl+,</td></tr>"
        "<tr><td>帮助</td><td>F1</td></tr>"
        "</table>";

    QMessageBox::information(this, "快捷键", shortcuts);
}

/**
 * @brief 关于
 */
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "关于",
        "<h2>GenPreCVSystem</h2>"
        "<p>版本: 1.0.0</p>"
        "<p>一个工业级计算机视觉预处理系统</p>"
        "<p><b>支持的格式：</b>PNG, JPG, JPEG, BMP, GIF, TIFF, WEBP 等图片格式</p>"
        "<hr>"
        "<p><b>操作说明：</b></p>"
        "<ul>"
        "<li><b>滚轮：</b>缩放图片</li>"
        "<li><b>左键拖拽：</b>平移图片</li>"
        "<li><b>双击文件：</b>加载图片</li>"
        "</ul>"
        "<hr>"
        "<p style='color: gray;'>© 2024 GenPreCVSystem. All rights reserved.</p>");
}

// ==================== 自定义槽函数 ====================

/**
 * @brief 向上一级目录导航
 */
void MainWindow::navigateUp()
{
    if (m_currentBrowsePath.isEmpty() || m_currentBrowsePath == QDir::rootPath()) {
        logMessage("已经到达根目录");
        return;
    }

    QDir dir(m_currentBrowsePath);
    if (dir.cdUp()) {
        QString parentPath = dir.absolutePath();
        QModelIndex sourceIndex = fileModel->index(parentPath);
        QModelIndex proxyIndex = proxyModel->mapFromSource(sourceIndex);

        treeViewFiles->setRootIndex(proxyIndex);
        m_currentBrowsePath = parentPath;
        labelCurrentPath->setText(m_currentBrowsePath);
        logMessage(QString("向上导航: %1").arg(parentPath));
    }
}

/**
 * @brief 文件树双击事件
 */
void MainWindow::onFileTreeDoubleClicked(const QModelIndex &index)
{
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    QString filePath = fileModel->filePath(sourceIndex);

    if (fileModel->isDir(sourceIndex)) {
        // 双击的是文件夹，进入该文件夹
        treeViewFiles->setRootIndex(index);
        m_currentBrowsePath = filePath;
        labelCurrentPath->setText(filePath);
        logMessage(QString("进入目录: %1").arg(filePath));
    } else {
        // 双击的是文件，加载图片
        loadImage(filePath);
    }
}

// ==================== 文件树右键菜单函数 ====================

/**
 * @brief 文件树右键菜单请求处理
 */
void MainWindow::onFileTreeContextMenuRequested(const QPoint &pos)
{
    QModelIndex proxyIndex = treeViewFiles->indexAt(pos);

    if (!proxyIndex.isValid()) {
        // 点击在空白处
        showEmptySpaceContextMenu(treeViewFiles->viewport()->mapToGlobal(pos));
        return;
    }

    m_contextMenuIndex = proxyIndex;
    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);

    if (fileModel->isDir(sourceIndex)) {
        QString folderPath = fileModel->filePath(sourceIndex);
        showFolderContextMenu(treeViewFiles->viewport()->mapToGlobal(pos), folderPath);
    } else {
        QString filePath = fileModel->filePath(sourceIndex);
        showFileContextMenu(treeViewFiles->viewport()->mapToGlobal(pos), filePath);
    }
}

/**
 * @brief 显示文件右键菜单
 */
void MainWindow::showFileContextMenu(const QPoint &globalPos, const QString &filePath)
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #ffffff; color: #000000; border: 1px solid #0066cc; }"
        "QMenu::item { padding: 6px 30px 6px 20px; }"
        "QMenu::item:selected { background-color: #0066cc; color: #ffffff; }"
        "QMenu::separator { height: 1px; background-color: #c0c0c0; margin: 4px 8px; }"
    );

    QAction *actionOpen = menu.addAction("打开图片");
    QAction *actionShowInExplorer = menu.addAction("在文件夹中显示");
    menu.addSeparator();
    QAction *actionCopyPath = menu.addAction("复制文件路径");
    QAction *actionCopyImage = menu.addAction("复制图片");
    menu.addSeparator();

    // 检查剪贴板是否有图片
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    bool hasImage = mimeData && (mimeData->hasImage() || mimeData->hasUrls());
    QAction *actionPaste = menu.addAction("粘贴图片");
    actionPaste->setEnabled(hasImage);

    menu.addSeparator();
    QAction *actionRename = menu.addAction("重命名");
    QAction *actionDelete = menu.addAction("删除文件");

    QAction *selectedAction = menu.exec(globalPos);

    if (selectedAction == actionOpen) {
        onOpenImage();
    } else if (selectedAction == actionShowInExplorer) {
        onShowInExplorer();
    } else if (selectedAction == actionCopyPath) {
        onCopyFilePath();
    } else if (selectedAction == actionCopyImage) {
        onCopyFile();
    } else if (selectedAction == actionPaste) {
        onPasteFromFile();
    } else if (selectedAction == actionRename) {
        onRenameFile();
    } else if (selectedAction == actionDelete) {
        onDeleteFile();
    }
}

/**
 * @brief 显示文件夹右键菜单
 */
void MainWindow::showFolderContextMenu(const QPoint &globalPos, const QString &folderPath)
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #ffffff; color: #000000; border: 1px solid #0066cc; }"
        "QMenu::item { padding: 6px 30px 6px 20px; }"
        "QMenu::item:selected { background-color: #0066cc; color: #ffffff; }"
        "QMenu::separator { height: 1px; background-color: #c0c0c0; margin: 4px 8px; }"
    );

    QAction *actionOpen = menu.addAction("打开文件夹");
    QAction *actionShowInExplorer = menu.addAction("在文件夹中显示");
    menu.addSeparator();
    QAction *actionCopyPath = menu.addAction("复制路径");

    // 检查剪贴板是否有图片
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    bool hasImage = mimeData && (mimeData->hasImage() || mimeData->hasUrls());
    QAction *actionPaste = menu.addAction("粘贴图片");
    actionPaste->setEnabled(hasImage);

    QAction *actionRefresh = menu.addAction("刷新");
    menu.addSeparator();
    QAction *actionDelete = menu.addAction("删除文件夹");

    QAction *selectedAction = menu.exec(globalPos);

    if (selectedAction == actionOpen) {
        onOpenFolder();
    } else if (selectedAction == actionShowInExplorer) {
        onShowInExplorer();
    } else if (selectedAction == actionCopyPath) {
        onCopyFilePath();
    } else if (selectedAction == actionPaste) {
        onPasteFromFile();
    } else if (selectedAction == actionRefresh) {
        onRefreshFolder();
    } else if (selectedAction == actionDelete) {
        onDeleteFolder();
    }
}

/**
 * @brief 显示空白处右键菜单
 */
void MainWindow::showEmptySpaceContextMenu(const QPoint &globalPos)
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #ffffff; color: #000000; border: 1px solid #0066cc; }"
        "QMenu::item { padding: 6px 30px 6px 20px; }"
        "QMenu::item:selected { background-color: #0066cc; color: #ffffff; }"
        "QMenu::separator { height: 1px; background-color: #c0c0c0; margin: 4px 8px; }"
    );

    // 检查剪贴板是否有图片
    QMimeData *mimeData = const_cast<QMimeData*>(QApplication::clipboard()->mimeData());
    bool hasImage = mimeData && (mimeData->hasImage() || mimeData->hasUrls());

    QAction *actionPaste = menu.addAction("粘贴图片");
    actionPaste->setEnabled(hasImage);
    menu.addSeparator();
    QAction *actionRefresh = menu.addAction("刷新");
    QAction *actionNewFolder = menu.addAction("新建文件夹");

    QAction *selectedAction = menu.exec(globalPos);

    if (selectedAction == actionPaste) {
        onPasteFromFile();
    } else if (selectedAction == actionRefresh) {
        onRefreshView();
    } else if (selectedAction == actionNewFolder) {
        onCreateNewFolder();
    }
}

// ==================== 文件树右键菜单槽函数 ====================

/**
 * @brief 打开图片（从右键菜单）
 */
void MainWindow::onOpenImage()
{
    if (!m_contextMenuIndex.isValid()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    QString filePath = fileModel->filePath(sourceIndex);
    loadImage(filePath);
}

/**
 * @brief 在文件夹中显示（支持文件和文件夹）
 */
void MainWindow::onShowInExplorer()
{
    if (!m_contextMenuIndex.isValid()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    QString path = fileModel->filePath(sourceIndex);

    #ifdef Q_OS_WIN
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(path);
    QProcess::startDetached("explorer", args);
    #else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
    #endif

    logMessage(QString("在资源管理器中显示: %1").arg(path));
}

/**
 * @brief 复制文件/文件夹路径
 */
void MainWindow::onCopyFilePath()
{
    if (!m_contextMenuIndex.isValid()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    QString path = fileModel->filePath(sourceIndex);

    QApplication::clipboard()->setText(path);
    logMessage(QString("已复制路径: %1").arg(path));
}

/**
 * @brief 复制文件到剪贴板（同时复制图片数据和文件引用）
 */
void MainWindow::onCopyFile()
{
    if (!m_contextMenuIndex.isValid()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    QString filePath = fileModel->filePath(sourceIndex);

    // 加载图片
    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片文件");
        return;
    }

    // 创建 QMimeData 同时支持两种格式
    QMimeData *mimeData = new QMimeData();

    // 1. 设置图片数据 - 用于在图片编辑器、Word等应用中粘贴图片内容
    mimeData->setImageData(pixmap.toImage());

    // 2. 设置文件引用 - 用于在文件管理器中粘贴文件副本
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(filePath);
    mimeData->setUrls(urls);

    QApplication::clipboard()->setMimeData(mimeData);
    logMessage(QString("已复制图片和文件: %1").arg(QFileInfo(filePath).fileName()));
}

/**
 * @brief 删除文件
 */
void MainWindow::onDeleteFile()
{
    if (!m_contextMenuIndex.isValid()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    QString filePath = fileModel->filePath(sourceIndex);
    QFileInfo fileInfo(filePath);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        QString("确定要删除文件 \"%1\" 吗？").arg(fileInfo.fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );

    if (reply == QMessageBox::No) return;

    if (QFile::remove(filePath)) {
        logMessage(QString("已删除文件: %1").arg(filePath));
        if (filePath == m_currentImagePath) {
            closeImage();
        }
    } else {
        QMessageBox::critical(this, "删除失败", "无法删除文件。");
    }
}

/**
 * @brief 重命名文件
 */
void MainWindow::onRenameFile()
{
    if (!m_contextMenuIndex.isValid()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    QString filePath = fileModel->filePath(sourceIndex);
    QFileInfo fileInfo(filePath);

    bool ok;
    QString newName = QInputDialog::getText(
        this, "重命名文件", "新文件名:",
        QLineEdit::Normal, fileInfo.fileName(), &ok
    );

    if (!ok || newName.isEmpty() || newName == fileInfo.fileName()) return;

    QString newPath = fileInfo.absolutePath() + "/" + newName;

    if (QFile::exists(newPath)) {
        QMessageBox::warning(this, "错误", "目标文件名已存在。");
        return;
    }

    QFile file(filePath);
    if (file.rename(newPath)) {
        logMessage(QString("文件已重命名: %1 -> %2").arg(fileInfo.fileName(), newName));
        if (filePath == m_currentImagePath) {
            m_currentImagePath = newPath;
            if (m_taskController) {
                m_taskController->setCurrentImagePath(newPath);
            }
        }
    } else {
        QMessageBox::critical(this, "重命名失败", "无法重命名文件。");
    }
}

/**
 * @brief 打开文件夹（从右键菜单）
 */
void MainWindow::onOpenFolder()
{
    if (!m_contextMenuIndex.isValid()) return;

    treeViewFiles->setRootIndex(m_contextMenuIndex);
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    QString folderPath = fileModel->filePath(sourceIndex);
    m_currentBrowsePath = folderPath;
    labelCurrentPath->setText(folderPath);

    logMessage(QString("进入目录: %1").arg(folderPath));
}

/**
 * @brief 刷新文件夹
 */
void MainWindow::onRefreshFolder()
{
    if (!m_contextMenuIndex.isValid()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    // Qt 6中 QFileSystemModel 会自动监视文件系统变化，无需手动刷新
    // 触发目录重新设置来强制刷新
    QString path = fileModel->filePath(sourceIndex);
    fileModel->setRootPath(path);
    logMessage("文件夹已刷新");
}

/**
 * @brief 删除文件夹
 */
void MainWindow::onDeleteFolder()
{
    if (!m_contextMenuIndex.isValid()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(m_contextMenuIndex);
    QString folderPath = fileModel->filePath(sourceIndex);
    QFileInfo folderInfo(folderPath);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        QString("确定要删除文件夹 \"%1\" 及其所有内容吗？").arg(folderInfo.fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );

    if (reply == QMessageBox::No) return;

    QDir dir(folderPath);
    if (dir.removeRecursively()) {
        logMessage(QString("已删除文件夹: %1").arg(folderPath));
    } else {
        QMessageBox::critical(this, "删除失败", "无法删除文件夹。");
    }
}

/**
 * @brief 刷新当前视图
 */
void MainWindow::onRefreshView()
{
    // Qt 6中 QFileSystemModel 会自动监视文件系统变化，无需手动刷新
    // 触发目录重新设置来强制刷新
    if (!m_currentBrowsePath.isEmpty()) {
        fileModel->setRootPath(m_currentBrowsePath);
    }
    logMessage("视图已刷新");
}

/**
 * @brief 新建文件夹
 */
void MainWindow::onCreateNewFolder()
{
    bool ok;
    QString folderName = QInputDialog::getText(
        this, "新建文件夹", "文件夹名称:",
        QLineEdit::Normal, "新建文件夹", &ok
    );

    if (!ok || folderName.isEmpty()) return;

    QDir currentDir(m_currentBrowsePath);
    QString newFolderPath = currentDir.absoluteFilePath(folderName);

    if (QFileInfo::exists(newFolderPath)) {
        QMessageBox::warning(this, "错误", "同名文件夹已存在。");
        return;
    }

    if (currentDir.mkdir(folderName)) {
        logMessage(QString("已创建文件夹: %1").arg(folderName));
    } else {
        QMessageBox::critical(this, "创建失败", "无法创建文件夹。");
    }
}

/**
 * @brief 从剪贴板粘贴图片到当前目录
 */
void MainWindow::onPasteFromFile()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData) {
        QMessageBox::warning(this, "提示", "剪贴板为空");
        return;
    }

    QPixmap pixmap;
    QString sourceFileName;

    // 优先从 URL 获取文件（保留原文件名）
    if (mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);

                // 检查是否是图片文件
                if (QPixmap(filePath).isNull()) {
                    continue;
                }

                pixmap = QPixmap(filePath);
                sourceFileName = fileInfo.fileName();
                break;
            }
        }
    }

    // 如果没有从 URL 获取到图片，尝试直接获取图片数据
    if (pixmap.isNull() && mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            pixmap = QPixmap::fromImage(image);
            // 使用时间戳生成文件名
            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            sourceFileName = QString("pasted_%1.png").arg(timestamp);
        }
    }

    if (pixmap.isNull()) {
        QMessageBox::warning(this, "提示", "剪贴板中没有图片");
        return;
    }

    // 确定保存路径
    QDir currentDir(m_currentBrowsePath.isEmpty() ? QDir::homePath() : m_currentBrowsePath);
    QString targetPath = currentDir.absoluteFilePath(sourceFileName);

    // 检查文件是否存在
    if (QFileInfo::exists(targetPath)) {
        QString previewPath = generateCopyFilePath(targetPath);
        QString previewName = QFileInfo(previewPath).fileName();

        int choice = showSaveOptionsDialog(previewName);

        if (choice == -1) {
            return;  // 用户取消
        } else if (choice == 1) {
            targetPath = previewPath;  // 保存副本
        }
        // choice == 0: 覆盖原文件，使用原路径
    }

    // 保存图片
    if (pixmap.save(targetPath)) {
        logMessage(QString("已粘贴图片: %1").arg(QFileInfo(targetPath).fileName()));

        // 刷新视图以显示新文件
        fileModel->setRootPath(m_currentBrowsePath);
    } else {
        QMessageBox::critical(this, "错误", "保存图片失败");
    }
}

/**
 * @brief 处理拖放的图片文件
 */
void MainWindow::onImagesDropped(const QStringList &filePaths)
{
    QDir currentDir(m_currentBrowsePath.isEmpty() ? QDir::homePath() : m_currentBrowsePath);

    for (const QString &sourcePath : filePaths) {
        QFileInfo sourceInfo(sourcePath);
        QString targetPath = currentDir.absoluteFilePath(sourceInfo.fileName());

        // 检查文件是否存在
        if (QFileInfo::exists(targetPath)) {
            QString previewPath = generateCopyFilePath(targetPath);
            QString previewName = QFileInfo(previewPath).fileName();

            int choice = showSaveOptionsDialog(previewName);

            if (choice == -1) {
                continue;  // 用户取消，处理下一个文件
            } else if (choice == 1) {
                targetPath = previewPath;  // 保存副本
            }
            // choice == 0: 覆盖原文件，使用原路径
        }

        // 复制文件
        if (QFile::copy(sourcePath, targetPath)) {
            logMessage(QString("已拖放保存: %1").arg(QFileInfo(targetPath).fileName()));
        } else {
            QMessageBox::warning(this, "错误", QString("无法保存文件: %1").arg(sourceInfo.fileName()));
        }
    }

    // 刷新视图以显示新文件
    fileModel->setRootPath(m_currentBrowsePath);
}

/**
 * @brief 处理拖放的图片数据
 */
void MainWindow::onImageDataDropped(const QByteArray &imageData, const QString &sourceName)
{
    // 从字节数据加载图片
    QPixmap pixmap;
    pixmap.loadFromData(imageData);

    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载拖放的图片数据");
        return;
    }

    // 确定保存路径
    QDir currentDir(m_currentBrowsePath.isEmpty() ? QDir::homePath() : m_currentBrowsePath);
    QString targetPath = currentDir.absoluteFilePath(sourceName);

    // 检查文件是否存在
    if (QFileInfo::exists(targetPath)) {
        QString previewPath = generateCopyFilePath(targetPath);
        QString previewName = QFileInfo(previewPath).fileName();

        int choice = showSaveOptionsDialog(previewName);

        if (choice == -1) {
            return;  // 用户取消
        } else if (choice == 1) {
            targetPath = previewPath;  // 保存副本
        }
        // choice == 0: 覆盖原文件，使用原路径
    }

    // 保存图片
    if (pixmap.save(targetPath)) {
        logMessage(QString("已拖放保存: %1").arg(QFileInfo(targetPath).fileName()));

        // 刷新视图以显示新文件
        fileModel->setRootPath(m_currentBrowsePath);
    } else {
        QMessageBox::critical(this, "错误", "保存图片失败");
    }
}

/**
 * @brief 处理拖放的文件夹（打开文件夹）
 */
void MainWindow::onFolderDropped(const QString &folderPath)
{
    QFileInfo folderInfo(folderPath);

    if (!folderInfo.exists() || !folderInfo.isDir()) {
        QMessageBox::warning(this, "错误", "无效的文件夹路径");
        return;
    }

    // 更新文件浏览器根目录
    QModelIndex sourceIndex = fileModel->index(folderPath);
    QModelIndex proxyIndex = proxyModel->mapFromSource(sourceIndex);

    treeViewFiles->setRootIndex(proxyIndex);
    m_currentBrowsePath = folderPath;
    labelCurrentPath->setText(folderPath);

    logMessage(QString("已打开文件夹: %1").arg(folderPath));
}

// ==================== 撤销/重做辅助函数 ====================

/**
 * @brief 保存当前状态到撤销栈
 *
 * 在执行任何修改操作前调用此函数保存当前状态
 */
void MainWindow::saveState()
{
    if (m_currentPixmap.isNull()) {
        return;
    }

    // 将当前状态压入撤销栈
    m_undoStack.push(m_currentPixmap);

    // 限制栈的大小，避免内存占用过大
    if (m_undoStack.size() > MAX_UNDO_STEPS) {
        m_undoStack.remove(0);  // 移除最旧的状态
    }

    // 清空重做栈（新操作会使之前的重做历史失效）
    m_redoStack.clear();

    // 更新按钮状态
    updateUndoRedoState();
}

/**
 * @brief 更新撤销/重做按钮的启用状态
 */
void MainWindow::updateUndoRedoState()
{
    ui->actionUndo->setEnabled(!m_undoStack.isEmpty());
    ui->actionRedo->setEnabled(!m_redoStack.isEmpty());
}

// ==================== 保存辅助函数 ====================

/**
 * @brief 获取已存在的副本编号列表
 * @param baseName 基础文件名
 * @param dir 文件目录
 * @param suffix 文件扩展名
 * @return 已使用的副本编号列表
 */
QList<int> MainWindow::getExistingCopyNumbers(const QString &baseName,
                                              const QDir &dir,
                                              const QString &suffix)
{
    QList<int> numbers;

    // 构建正则表达式匹配副本文件
    QRegularExpression regex(QString("^%1_副本(\\d+)\\.%2$")
        .arg(QRegularExpression::escape(baseName))
        .arg(QRegularExpression::escape(suffix)));

    // 获取目录中所有文件
    QFileInfoList fileList = dir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot, QDir::NoSort
    );

    // 匹配并提取编号
    for (const QFileInfo &fileInfo : fileList) {
        QRegularExpressionMatch match = regex.match(fileInfo.fileName());
        if (match.hasMatch()) {
            int number = match.captured(1).toInt();
            numbers.append(number);
        }
    }

    // 排序编号列表
    std::sort(numbers.begin(), numbers.end());

    return numbers;
}

/**
 * @brief 生成副本文件路径
 * @param originalPath 原始文件路径
 * @return 副本文件路径
 */
QString MainWindow::generateCopyFilePath(const QString &originalPath)
{
    QFileInfo fileInfo(originalPath);
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.suffix();
    QDir dir = fileInfo.absoluteDir();

    // 获取已存在的副本编号
    QList<int> existingNumbers = getExistingCopyNumbers(baseName, dir, suffix);

    // 找到最小可用编号
    int copyNumber = 1;
    for (int num : existingNumbers) {
        if (num == copyNumber) {
            copyNumber++;
        } else {
            break; // 找到空缺编号
        }
    }

    // 构造新文件名
    QString newFileName = QString("%1_副本%2.%3")
        .arg(baseName)
        .arg(copyNumber)
        .arg(suffix);

    return dir.absoluteFilePath(newFileName);
}

/**
 * @brief 显示保存选项对话框
 * @param previewFileName 副本文件名预览
 * @return 0=覆盖, 1=保存副本, -1=取消
 */
int MainWindow::showSaveOptionsDialog(const QString &previewFileName)
{
    QDialog dialog(this);
    dialog.setWindowTitle("保存选项");
    dialog.resize(450, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    // 信息提示
    QLabel *infoLabel = new QLabel("文件已存在，请选择保存方式：", &dialog);
    mainLayout->addWidget(infoLabel);

    // 选项组
    QGroupBox *optionsGroup = new QGroupBox("保存选项", &dialog);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);

    QRadioButton *overwriteRadio = new QRadioButton("覆盖原文件", &dialog);
    QRadioButton *copyRadio = new QRadioButton("保存副本", &dialog);
    copyRadio->setChecked(true); // 默认选中保存副本

    optionsLayout->addWidget(overwriteRadio);
    optionsLayout->addWidget(copyRadio);

    // 副本文件名预览
    QWidget *previewWidget = new QWidget(&dialog);
    QHBoxLayout *previewLayout = new QHBoxLayout(previewWidget);
    previewLayout->setContentsMargins(20, 0, 0, 0);

    QLabel *previewLabel = new QLabel("新文件名:", &dialog);
    QLabel *previewValue = new QLabel(previewFileName, &dialog);
    previewValue->setStyleSheet("font-weight: bold; color: #0066cc;");

    previewLayout->addWidget(previewLabel);
    previewLayout->addWidget(previewValue);
    previewLayout->addStretch();

    optionsLayout->addWidget(previewWidget);

    mainLayout->addWidget(optionsGroup);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *saveButton = new QPushButton("保存", &dialog);
    QPushButton *cancelButton = new QPushButton("取消", &dialog);

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        // 返回用户选择
        if (overwriteRadio->isChecked()) {
            return 0; // 覆盖
        } else {
            return 1; // 保存副本
        }
    }

    return -1; // 取消
}

// ==================== 图像处理辅助函数 ====================

/**
 * @brief 应用卷积核到图片
 */
QImage MainWindow::applyConvolution(const QImage &image, const QVector<QVector<int>> &kernel, int divisor, int offset)
{
    if (image.isNull() || kernel.isEmpty() || kernel.size() % 2 == 0 || kernel[0].size() % 2 == 0) {
        return image;
    }

    int kernelSize = kernel.size();
    int half = kernelSize / 2;

    QImage result(image.size(), QImage::Format_RGB32);
    QImage src = image.convertToFormat(QImage::Format_RGB32);

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int r = 0, g = 0, b = 0;

            for (int ky = 0; ky < kernelSize; ++ky) {
                for (int kx = 0; kx < kernelSize; ++kx) {
                    int px = qBound(0, x + kx - half, image.width() - 1);
                    int py = qBound(0, y + ky - half, image.height() - 1);

                    QColor pixel = src.pixelColor(px, py);
                    int weight = kernel[ky][kx];

                    r += pixel.red() * weight;
                    g += pixel.green() * weight;
                    b += pixel.blue() * weight;
                }
            }

            r = qBound(0, r / divisor + offset, 255);
            g = qBound(0, g / divisor + offset, 255);
            b = qBound(0, b / divisor + offset, 255);

            result.setPixelColor(x, y, QColor(r, g, b));
        }
    }

    return result;
}

/**
 * @brief 高斯模糊
 */
QImage MainWindow::gaussianBlur(const QImage &image, int radius)
{
    // 使用简单的盒式模糊作为近似
    QImage result = image;
    int iterations = radius * 2;

    for (int i = 0; i < iterations; ++i) {
        QVector<QVector<int>> kernel = {
            {1, 2, 1},
            {2, 4, 2},
            {1, 2, 1}
        };

        QImage temp(result.size(), QImage::Format_RGB32);
        for (int y = 0; y < result.height(); ++y) {
            for (int x = 0; x < result.width(); ++x) {
                int r = 0, g = 0, b = 0, count = 0;

                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int px = qBound(0, x + kx, result.width() - 1);
                        int py = qBound(0, y + ky, result.height() - 1);
                        QColor pixel = result.pixelColor(px, py);
                        int weight = kernel[ky + 1][kx + 1];

                        r += pixel.red() * weight;
                        g += pixel.green() * weight;
                        b += pixel.blue() * weight;
                        count += weight;
                    }
                }

                temp.setPixelColor(x, y, QColor(r / count, g / count, b / count));
            }
        }

        result = temp;
    }

    return result;
}

/**
 * @brief 锐化图片
 */
QImage MainWindow::sharpenImage(const QImage &image, double strength)
{
    // 锐化卷积核
    int s = static_cast<int>(strength);
    QVector<QVector<int>> kernel = {
        {0,        -s,       0},
        {-s,   1 + 4*s,    -s},
        {0,        -s,       0}
    };

    return applyConvolution(image, kernel, 1);
}

// ==================== 任务栏和参数面板实现 ====================

/**
 * @brief 初始化任务菜单
 */
void MainWindow::setupTaskMenus()
{
    taskActionGroup = new QActionGroup(this);
    taskActionGroup->setExclusive(true);

    // 创建任务控制器
    m_taskController = new GenPreCVSystem::Controllers::TaskController(this);
    m_taskController->setParameterScrollArea(paramScrollArea);
    m_taskController->setTaskActionGroup(taskActionGroup);
    m_taskController->setTabWidget(tabWidget);  // 设置 TabWidget 用于获取当前 ImageView

    // 连接任务控制器的日志信号
    connect(m_taskController, &GenPreCVSystem::Controllers::TaskController::logMessage,
            this, &MainWindow::logMessage);

    // 使用UI中的菜单动作，设置数据以便识别任务类型
    ui->actionTaskImageClassification->setData(QVariant::fromValue(static_cast<int>(CVTask::ImageClassification)));
    ui->actionTaskObjectDetection->setData(QVariant::fromValue(static_cast<int>(CVTask::ObjectDetection)));
    ui->actionTaskSemanticSegmentation->setData(QVariant::fromValue(static_cast<int>(CVTask::SemanticSegmentation)));
    ui->actionTaskKeyPointDetection->setData(QVariant::fromValue(static_cast<int>(CVTask::KeyPointDetection)));
    ui->actionTaskImageEnhancement->setData(QVariant::fromValue(static_cast<int>(CVTask::ImageEnhancement)));
    ui->actionTaskImageDenoising->setData(QVariant::fromValue(static_cast<int>(CVTask::ImageDenoising)));
    ui->actionTaskEdgeDetection->setData(QVariant::fromValue(static_cast<int>(CVTask::EdgeDetection)));

    // 将菜单动作添加到动作组
    taskActionGroup->addAction(ui->actionTaskImageClassification);
    taskActionGroup->addAction(ui->actionTaskObjectDetection);
    taskActionGroup->addAction(ui->actionTaskSemanticSegmentation);
    taskActionGroup->addAction(ui->actionTaskKeyPointDetection);
    taskActionGroup->addAction(ui->actionTaskImageEnhancement);
    taskActionGroup->addAction(ui->actionTaskImageDenoising);
    taskActionGroup->addAction(ui->actionTaskEdgeDetection);

    // 连接信号
    connect(taskActionGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        CVTask task = static_cast<CVTask>(action->data().toInt());
        switchTask(task);
    });

    // 初始化参数面板
    updateParameterPanel(CVTask::ImageClassification);
}

/**
 * @brief 切换任务类型
 */
void MainWindow::switchTask(CVTask task)
{
    m_currentTask = task;
    updateParameterPanel(task);
    // 日志消息由 TaskController 发出（已连接到 MainWindow::logMessage）
}

/**
 * @brief 更新参数面板内容
 */
void MainWindow::updateParameterPanel(CVTask task)
{
    // 委托给 TaskController 处理（包含 YOLO 服务控件和信号连接）
    if (m_taskController) {
        // 将本地 CVTask 转换为 Models::CVTask
        auto modelsTask = static_cast<GenPreCVSystem::Models::CVTask>(static_cast<int>(task));
        m_taskController->switchTask(modelsTask);
    }
}

/**
 * @brief 清空参数面板
 */
void MainWindow::clearParameterPanel()
{
    if (paramScrollArea && paramScrollArea->widget()) {
        delete paramScrollArea->widget();
        paramScrollArea->setWidget(nullptr);
    }
}

/**
 * @brief 创建图像分类参数面板
 */
QWidget* MainWindow::createImageClassificationParams()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 模型选择
    QGroupBox *modelGroup = new QGroupBox("模型设置", widget);
    QFormLayout *modelLayout = new QFormLayout(modelGroup);
    QComboBox *modelCombo = new QComboBox(widget);
    modelCombo->addItems({"ResNet50", "EfficientNet", "ViT", "Swin Transformer"});
    modelLayout->addRow("模型:", modelCombo);
    layout->addWidget(modelGroup);

    // 训练参数
    QGroupBox *trainGroup = new QGroupBox("训练参数", widget);
    QFormLayout *trainLayout = new QFormLayout(trainGroup);

    QSpinBox *batchSpinBox = new QSpinBox(widget);
    batchSpinBox->setRange(1, 256);
    batchSpinBox->setValue(32);
    trainLayout->addRow("批处理大小:", batchSpinBox);

    QDoubleSpinBox *lrSpinBox = new QDoubleSpinBox(widget);
    lrSpinBox->setRange(0.0001, 1.0);
    lrSpinBox->setDecimals(4);
    lrSpinBox->setSingleStep(0.0001);
    lrSpinBox->setValue(0.001);
    trainLayout->addRow("学习率:", lrSpinBox);

    QSpinBox *epochSpinBox = new QSpinBox(widget);
    epochSpinBox->setRange(1, 1000);
    epochSpinBox->setValue(100);
    trainLayout->addRow("训练轮数:", epochSpinBox);

    QComboBox *optimCombo = new QComboBox(widget);
    optimCombo->addItems({"SGD", "Adam", "AdamW", "RMSprop"});
    trainLayout->addRow("优化器:", optimCombo);

    layout->addWidget(trainGroup);

    // 数据增强
    QGroupBox *augGroup = new QGroupBox("数据增强", widget);
    QVBoxLayout *augLayout = new QVBoxLayout(augGroup);

    QCheckBox *randomFlip = new QCheckBox("随机水平翻转", widget);
    randomFlip->setChecked(true);
    augLayout->addWidget(randomFlip);

    QCheckBox *randomRotate = new QCheckBox("随机旋转", widget);
    randomRotate->setChecked(true);
    augLayout->addWidget(randomRotate);

    QCheckBox *colorJitter = new QCheckBox("颜色抖动", widget);
    colorJitter->setChecked(true);
    augLayout->addWidget(colorJitter);

    QCheckBox *norm = new QCheckBox("标准化", widget);
    norm->setChecked(true);
    augLayout->addWidget(norm);

    layout->addWidget(augGroup);

    layout->addStretch();
    return widget;
}

/**
 * @brief 创建目标检测参数面板
 */
QWidget* MainWindow::createObjectDetectionParams()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 模型选择
    QGroupBox *modelGroup = new QGroupBox("模型设置", widget);
    QFormLayout *modelLayout = new QFormLayout(modelGroup);
    QComboBox *modelCombo = new QComboBox(widget);
    modelCombo->addItems({"YOLO v8", "Faster R-CNN", "SSD", "RT-DETR"});
    modelLayout->addRow("模型:", modelCombo);
    layout->addWidget(modelGroup);

    // 推理参数
    QGroupBox *inferGroup = new QGroupBox("推理参数", widget);
    QFormLayout *inferLayout = new QFormLayout(inferGroup);

    QDoubleSpinBox *confSpinBox = new QDoubleSpinBox(widget);
    confSpinBox->setRange(0.0, 1.0);
    confSpinBox->setDecimals(2);
    confSpinBox->setSingleStep(0.05);
    confSpinBox->setValue(0.25);
    inferLayout->addRow("置信度阈值:", confSpinBox);

    QDoubleSpinBox *iouSpinBox = new QDoubleSpinBox(widget);
    iouSpinBox->setRange(0.0, 1.0);
    iouSpinBox->setDecimals(2);
    iouSpinBox->setSingleStep(0.05);
    iouSpinBox->setValue(0.45);
    inferLayout->addRow("IOU阈值:", iouSpinBox);

    layout->addWidget(inferGroup);

    // 锚框设置
    QGroupBox *anchorGroup = new QGroupBox("锚框设置", widget);
    QFormLayout *anchorLayout = new QFormLayout(anchorGroup);

    QSpinBox *numAnchors = new QSpinBox(widget);
    numAnchors->setRange(1, 10);
    numAnchors->setValue(3);
    anchorLayout->addRow("锚框数量:", numAnchors);

    QComboBox *sizesCombo = new QComboBox(widget);
    sizesCombo->addItems({"COCO", "VOC", "Custom"});
    anchorLayout->addRow("预置尺寸:", sizesCombo);

    layout->addWidget(anchorGroup);

    layout->addStretch();
    return widget;
}

/**
 * @brief 创建语义分割参数面板
 */
QWidget* MainWindow::createSemanticSegmentationParams()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 模型选择
    QGroupBox *modelGroup = new QGroupBox("模型设置", widget);
    QFormLayout *modelLayout = new QFormLayout(modelGroup);
    QComboBox *modelCombo = new QComboBox(widget);
    modelCombo->addItems({"DeepLab V3+", "U-Net", "SegFormer", "PSPNet"});
    modelLayout->addRow("模型:", modelCombo);

    QSpinBox *inputSize = new QSpinBox(widget);
    inputSize->setRange(128, 1024);
    inputSize->setSingleStep(32);
    inputSize->setValue(512);
    modelLayout->addRow("输入尺寸:", inputSize);

    QSpinBox *numClasses = new QSpinBox(widget);
    numClasses->setRange(1, 1000);
    numClasses->setValue(21);
    modelLayout->addRow("类别数量:", numClasses);

    layout->addWidget(modelGroup);

    // 损失函数
    QGroupBox *lossGroup = new QGroupBox("损失函数", widget);
    QVBoxLayout *lossLayout = new QVBoxLayout(lossGroup);
    QComboBox *lossCombo = new QComboBox(widget);
    lossCombo->addItems({"Cross Entropy", "Focal Loss", "Dice Loss", "Combo Loss"});
    lossLayout->addWidget(new QLabel("损失类型:", widget));
    lossLayout->addWidget(lossCombo);
    layout->addWidget(lossGroup);

    layout->addStretch();
    return widget;
}

/**
 * @brief 创建关键点检测参数面板
 */
QWidget* MainWindow::createKeyPointDetectionParams()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 模型选择
    QGroupBox *modelGroup = new QGroupBox("模型设置", widget);
    QFormLayout *modelLayout = new QFormLayout(modelGroup);
    QComboBox *modelCombo = new QComboBox(widget);
    modelCombo->addItems({"MMPose", "OpenPose", "HRNet", "ViTPose"});
    modelLayout->addRow("模型:", modelCombo);

    QSpinBox *numPoints = new QSpinBox(widget);
    numPoints->setRange(1, 500);
    numPoints->setValue(17);
    modelLayout->addRow("关键点数量:", numPoints);

    layout->addWidget(modelGroup);

    // 推理参数
    QGroupBox *inferGroup = new QGroupBox("推理参数", widget);
    QFormLayout *inferLayout = new QFormLayout(inferGroup);

    QDoubleSpinBox *confSpinBox = new QDoubleSpinBox(widget);
    confSpinBox->setRange(0.0, 1.0);
    confSpinBox->setDecimals(2);
    confSpinBox->setValue(0.3);
    inferLayout->addRow("置信度阈值:", confSpinBox);

    QCheckBox *tracking = new QCheckBox("启用跟踪", widget);
    inferLayout->addRow("", tracking);

    layout->addWidget(inferGroup);

    layout->addStretch();
    return widget;
}

/**
 * @brief 创建图像增强参数面板
 */
QWidget* MainWindow::createImageEnhancementParams()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 亮度调整
    QGroupBox *brightnessGroup = new QGroupBox("亮度调整", widget);
    QVBoxLayout *brightnessLayout = new QVBoxLayout(brightnessGroup);
    QSlider *brightnessSlider = new QSlider(Qt::Horizontal, widget);
    brightnessSlider->setRange(-100, 100);
    brightnessSlider->setValue(0);
    QLabel *brightnessValue = new QLabel("0", widget);
    connect(brightnessSlider, &QSlider::valueChanged, [brightnessValue](int value) {
        brightnessValue->setText(QString::number(value));
    });
    QHBoxLayout *brightnessControl = new QHBoxLayout();
    brightnessControl->addWidget(brightnessSlider);
    brightnessControl->addWidget(brightnessValue);
    brightnessLayout->addLayout(brightnessControl);
    layout->addWidget(brightnessGroup);

    // 对比度调整
    QGroupBox *contrastGroup = new QGroupBox("对比度调整", widget);
    QVBoxLayout *contrastLayout = new QVBoxLayout(contrastGroup);
    QSlider *contrastSlider = new QSlider(Qt::Horizontal, widget);
    contrastSlider->setRange(-100, 100);
    contrastSlider->setValue(0);
    QLabel *contrastValue = new QLabel("0", widget);
    connect(contrastSlider, &QSlider::valueChanged, [contrastValue](int value) {
        contrastValue->setText(QString::number(value));
    });
    QHBoxLayout *contrastControl = new QHBoxLayout();
    contrastControl->addWidget(contrastSlider);
    contrastControl->addWidget(contrastValue);
    contrastLayout->addLayout(contrastControl);
    layout->addWidget(contrastGroup);

    // 饱和度调整
    QGroupBox *satGroup = new QGroupBox("饱和度调整", widget);
    QVBoxLayout *satLayout = new QVBoxLayout(satGroup);
    QSlider *satSlider = new QSlider(Qt::Horizontal, widget);
    satSlider->setRange(-100, 100);
    satSlider->setValue(0);
    QLabel *satValue = new QLabel("0", widget);
    connect(satSlider, &QSlider::valueChanged, [satValue](int value) {
        satValue->setText(QString::number(value));
    });
    QHBoxLayout *satControl = new QHBoxLayout();
    satControl->addWidget(satSlider);
    satControl->addWidget(satValue);
    satLayout->addLayout(satControl);
    layout->addWidget(satGroup);

    // 锐化
    QGroupBox *sharpGroup = new QGroupBox("锐化", widget);
    QVBoxLayout *sharpLayout = new QVBoxLayout(sharpGroup);
    QSlider *sharpSlider = new QSlider(Qt::Horizontal, widget);
    sharpSlider->setRange(0, 100);
    sharpSlider->setValue(0);
    QLabel *sharpValue = new QLabel("0", widget);
    connect(sharpSlider, &QSlider::valueChanged, [sharpValue](int value) {
        sharpValue->setText(QString::number(value));
    });
    QHBoxLayout *sharpControl = new QHBoxLayout();
    sharpControl->addWidget(sharpSlider);
    sharpControl->addWidget(sharpValue);
    sharpLayout->addLayout(sharpControl);
    layout->addWidget(sharpGroup);

    layout->addStretch();
    return widget;
}

/**
 * @brief 创建图像去噪参数面板
 */
QWidget* MainWindow::createImageDenoisingParams()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 去噪方法
    QGroupBox *methodGroup = new QGroupBox("去噪方法", widget);
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    QComboBox *methodCombo = new QComboBox(widget);
    methodCombo->addItems({"高斯滤波", "双边滤波", "非局部均值", "小波去噪"});
    methodLayout->addWidget(new QLabel("算法:", widget));
    methodLayout->addWidget(methodCombo);
    layout->addWidget(methodGroup);

    // 参数调整
    QGroupBox *paramGroup = new QGroupBox("参数调整", widget);
    QFormLayout *paramLayout = new QFormLayout(paramGroup);

    QSpinBox *kernelSize = new QSpinBox(widget);
    kernelSize->setRange(1, 15);
    kernelSize->setSingleStep(2);
    kernelSize->setValue(3);
    paramLayout->addRow("卷积核大小:", kernelSize);

    QDoubleSpinBox *sigma = new QDoubleSpinBox(widget);
    sigma->setRange(0.1, 10.0);
    sigma->setDecimals(1);
    sigma->setValue(1.0);
    paramLayout->addRow("Sigma值:", sigma);

    layout->addWidget(paramGroup);

    layout->addStretch();
    return widget;
}

/**
 * @brief 创建边缘检测参数面板
 */
QWidget* MainWindow::createEdgeDetectionParams()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 检测方法
    QGroupBox *methodGroup = new QGroupBox("检测方法", widget);
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    QComboBox *methodCombo = new QComboBox(widget);
    methodCombo->addItems({"Sobel", "Canny", "Laplacian", "Scharr"});
    methodLayout->addWidget(new QLabel("算法:", widget));
    methodLayout->addWidget(methodCombo);
    layout->addWidget(methodGroup);

    // Canny参数
    QGroupBox *cannyGroup = new QGroupBox("Canny参数", widget);
    QFormLayout *cannyLayout = new QFormLayout(cannyGroup);

    QDoubleSpinBox *threshold1 = new QDoubleSpinBox(widget);
    threshold1->setRange(0, 500);
    threshold1->setValue(100);
    cannyLayout->addRow("低阈值:", threshold1);

    QDoubleSpinBox *threshold2 = new QDoubleSpinBox(widget);
    threshold2->setRange(0, 500);
    threshold2->setValue(200);
    cannyLayout->addRow("高阈值:", threshold2);

    QSpinBox *aperture = new QSpinBox(widget);
    aperture->setRange(3, 7);
    aperture->setSingleStep(2);
    aperture->setValue(3);
    cannyLayout->addRow("孔径大小:", aperture);

    layout->addWidget(cannyGroup);

    // Sobel参数
    QGroupBox *sobelGroup = new QGroupBox("Sobel参数", widget);
    QFormLayout *sobelLayout = new QFormLayout(sobelGroup);

    QSpinBox *ksize = new QSpinBox(widget);
    ksize->setRange(1, 7);
    ksize->setSingleStep(2);
    ksize->setValue(3);
    sobelLayout->addRow("卷积核大小:", ksize);

    layout->addWidget(sobelGroup);

    layout->addStretch();
    return widget;
}

// ==================== 标签页槽函数实现 ====================

/**
 * @brief 标签页关闭请求
 * @param index 要关闭的标签页索引
 */
void MainWindow::onTabCloseRequested(int index)
{
    if (index < 0 || index >= tabWidget->count()) {
        return;
    }

    // 移除标签页数据
    m_tabData.remove(index);

    // 移除标签页
    tabWidget->removeTab(index);

    // 重新映射标签页数据索引
    QHash<int, TabData> newTabData;
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (m_tabData.contains(i)) {
            newTabData[i] = m_tabData[i];
        }
    }
    m_tabData = newTabData;

    logMessage("已关闭标签页");
}

/**
 * @brief 当前标签页改变
 * @param index 新的当前标签页索引
 */
void MainWindow::onCurrentTabChanged(int index)
{
    updateCurrentTabRef();

    if (index >= 0 && m_tabData.contains(index)) {
        const TabData &tabData = m_tabData[index];
        logMessage(QString("切换到: %1").arg(QFileInfo(tabData.imagePath).fileName()));
    }

    updateUndoRedoState();
}

/**
 * @brief 获取当前活动的ImageView
 */
ImageView* MainWindow::currentImageView() const
{
    if (tabWidget->currentIndex() >= 0) {
        return qobject_cast<ImageView*>(tabWidget->currentWidget());
    }
    return nullptr;
}

/**
 * @brief 更新当前标签页引用
 */
void MainWindow::updateCurrentTabRef()
{
    int index = tabWidget->currentIndex();
    if (index >= 0 && m_tabData.contains(index)) {
        const TabData &tabData = m_tabData[index];
        m_currentImagePath = tabData.imagePath;
        m_currentPixmap = tabData.pixmap;
        m_undoStack = tabData.undoStack;
        m_redoStack = tabData.redoStack;
    } else {
        m_currentImagePath.clear();
        m_currentPixmap = QPixmap();
        m_undoStack.clear();
        m_redoStack.clear();
    }

    // 通知任务控制器当前图像路径变化
    if (m_taskController) {
        m_taskController->setCurrentImagePath(m_currentImagePath);
    }
}
