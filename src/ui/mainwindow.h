#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QTextEdit>
#include <QTreeView>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QPixmap>
#include <QLabel>
#include <QStack>
#include <QRadioButton>
#include <QRegularExpression>
#include <QMenu>
#include <QProcess>

// 前向声明
namespace GenPreCVSystem {
namespace Controllers {
class TaskController;
}
namespace Utils {
class RecentFilesManager;
}
namespace Views {
class BatchProcessDialog;
}
}

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief CV任务类型枚举
 */
enum class CVTask {
    ImageClassification,      ///< 图像分类
    ObjectDetection,          ///< 目标检测
    SemanticSegmentation,     ///< 语义分割
    KeyPointDetection,        ///< 关键点检测
    ImageEnhancement,         ///< 图像增强
    ImageDenoising,           ///< 图像去噪
    EdgeDetection             ///< 边缘检测
};

/**
 * @brief 图片显示视图
 *
 * 继承自 QGraphicsView，实现图片显示、缩放、平移等功能
 */
class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);

    /**
     * @brief 设置要显示的图片
     * @param pixmap 图片数据
     */
    void setPixmap(const QPixmap &pixmap);

    /**
     * @brief 清空显示的图片
     */
    void clearImage();

    /**
     * @brief 缩放图片
     * @param factor 缩放因子（>1 放大，<1 缩小）
     */
    void scaleImage(double factor);

    /**
     * @brief 自适应窗口大小显示图片
     */
    void fitToWindow();

    /**
     * @brief 获取当前缩放比例
     * @return 当前缩放比例
     */
    double currentScale() const { return m_scaleFactor; }

    /**
     * @brief 缩放至实际大小
     */
    void actualSize();

    /**
     * @brief 获取当前显示的图片
     * @return 当前图片，如果没有图片则返回空 QPixmap
     */
    QPixmap pixmap() const;

protected:
    /**
     * @brief 鼠标滚轮事件，用于缩放图片
     */
    void wheelEvent(QWheelEvent *event) override;

    /**
     * @brief 鼠标按下事件，用于开始拖拽
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标移动事件，用于拖拽图片
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标释放事件，用于结束拖拽
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * @brief 调整大小事件，用于重新适应窗口
     */
    void resizeEvent(QResizeEvent *event) override;

private:
    QGraphicsScene *m_scene;              ///< 图形场景
    QGraphicsPixmapItem *m_pixmapItem;    ///< 图片项
    double m_scaleFactor;                 ///< 当前缩放比例
    bool m_dragging;                      ///< 是否正在拖拽
    QPoint m_lastPanPoint;                ///< 上一次拖拽位置
};

/**
 * @brief 文件树视图，支持拖放图片
 *
 * 继承自 QTreeView，处理外部文件拖放
 */
class FileTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit FileTreeView(QWidget *parent = nullptr);

    void setTargetPath(const QString &path) { m_targetPath = path; }
    QString targetPath() const { return m_targetPath; }

signals:
    /**
     * @brief 图片文件被拖放到视图
     * @param filePaths 图片文件路径列表
     */
    void imagesDropped(const QStringList &filePaths);
    /**
     * @brief 图片数据被拖放到视图
     * @param imageData 图片数据
     * @param sourceName 源名称（如果有）
     */
    void imageDataDropped(const QByteArray &imageData, const QString &sourceName);
    /**
     * @brief 文件夹被拖放到视图
     * @param folderPath 文件夹路径
     */
    void folderDropped(const QString &folderPath);

protected:
    /**
     * @brief 拖放进入事件
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /**
     * @brief 拖放移动事件
     */
    void dragMoveEvent(QDragMoveEvent *event) override;

    /**
     * @brief 拖放释放事件
     */
    void dropEvent(QDropEvent *event) override;

private:
    QString m_targetPath;  ///< 目标保存路径
};

/**
 * @brief 图片文件过滤代理模型
 *
 * 继承自 QSortFilterProxyModel，用于过滤文件系统模型，
 * 只显示图片格式的文件（如 png, jpg, jpeg, bmp, gif, tiff 等）
 */
class ImageFileFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ImageFileFilterProxyModel(QObject *parent = nullptr);

protected:
    /**
     * @brief 重写过滤函数，判断是否接受该行
     * @param sourceRow 源模型行号
     * @param sourceParent 源模型父索引
     * @return true 接受该行，false 过滤掉该行
     */
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

/**
 * @brief 主窗口类
 *
 * 实现类似 IDE/VSCode 的工业软件布局，包含：
 * - 顶部菜单栏（文件、编辑、视图、图像、工具、帮助）
 * - 左侧文件浏览器（可停靠，仅显示图片文件）
 * - 中间主工作区（图片展示器，支持缩放、平移）
 * - 右侧参数设置面板（可停靠，预留）
 * - 底部日志输出区域（可停靠）
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow();

private slots:
    // ========== 文件菜单槽函数 ==========

    void on_actionOpenImage_triggered();
    void on_actionOpenFolder_triggered();
    void on_actionCloseImage_triggered();
    void on_actionSaveImage_triggered();
    void on_actionSaveImageAs_triggered();
    void on_actionExport_triggered();
    void on_actionExit_triggered();

    // ========== 编辑菜单槽函数 ==========

    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCopyImage_triggered();
    void on_actionPaste_triggered();
    void on_actionRotateLeft_triggered();
    void on_actionRotateRight_triggered();
    void on_actionFlipHorizontal_triggered();
    void on_actionFlipVertical_triggered();

    // ========== 视图菜单槽函数 ==========

    void on_actionZoomIn_triggered();
    void on_actionZoomOut_triggered();
    void on_actionFitToWindow_triggered();
    void on_actionActualSize_triggered();
    void on_actionShowFileBrowser_triggered(bool checked);
    void on_actionShowParameterPanel_triggered(bool checked);
    void on_actionShowLogOutput_triggered(bool checked);

    // ========== 图像菜单槽函数 ==========

    void on_actionGrayscale_triggered(bool checked);
    void on_actionInvert_triggered(bool checked);
    void on_actionBlur_triggered();
    void on_actionSharpen_triggered();
    void on_actionThreshold_triggered();

    // ========== 任务菜单槽函数 ==========

    void on_actionTaskImageClassification_triggered();
    void on_actionTaskObjectDetection_triggered();
    void on_actionTaskSemanticSegmentation_triggered();
    void on_actionTaskKeyPointDetection_triggered();
    void on_actionTaskImageEnhancement_triggered();
    void on_actionTaskImageDenoising_triggered();
    void on_actionTaskEdgeDetection_triggered();

    // ========== 工具菜单槽函数 ==========

    void on_actionSettings_triggered();
    void on_actionBatchProcess_triggered();

    // ========== 帮助菜单槽函数 ==========

    void on_actionDocumentation_triggered();
    void on_actionShortcuts_triggered();
    void on_actionAbout_triggered();

    // ========== 标签页槽函数 ==========

    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);

    // ========== 自定义槽函数 ==========

    /**
     * @brief 文件树双击事件
     * @param index 被双击项的模型索引
     */
    void onFileTreeDoubleClicked(const QModelIndex &index);

    /**
     * @brief 向上一级目录导航
     */
    void navigateUp();

    // ========== 文件树右键菜单槽函数 ==========

    /**
     * @brief 文件树自定义上下文菜单信号槽
     */
    void onFileTreeContextMenuRequested(const QPoint &pos);

    /**
     * @brief 文件右键菜单槽函数组
     */
    void onOpenImage();
    void onShowInExplorer();
    void onCopyFilePath();
    void onCopyFile();
    void onDeleteFile();
    void onRenameFile();

    /**
     * @brief 文件夹右键菜单槽函数组
     */
    void onOpenFolder();
    void onRefreshFolder();
    void onDeleteFolder();

    /**
     * @brief 通用菜单槽函数
     */
    void onRefreshView();
    void onCreateNewFolder();
    void onPasteFromFile();

    /**
     * @brief 拖放槽函数
     */
    void onImagesDropped(const QStringList &filePaths);
    void onImageDataDropped(const QByteArray &imageData, const QString &sourceName);
    void onFolderDropped(const QString &folderPath);

    /**
     * @brief 最近文件被点击
     */
    void onRecentFileTriggered(const QString &filePath);

private:
    Ui::MainWindow *ui;  ///< UI 界面对象

    // ========== 停靠窗口 ==========

    QDockWidget *dockFileBrowser;  ///< 左侧文件浏览器停靠窗口
    QDockWidget *dockParameters;   ///< 右侧参数设置停靠窗口
    QDockWidget *dockLogOutput;    ///< 底部日志输出停靠窗口

    // ========== 任务菜单 ==========

    QActionGroup *taskActionGroup; ///< 任务动作组（互斥选择）
    CVTask m_currentTask;          ///< 当前选中的任务
    GenPreCVSystem::Controllers::TaskController *m_taskController; ///< 任务控制器
    GenPreCVSystem::Utils::RecentFilesManager *m_recentFilesManager; ///< 最近文件管理器
    GenPreCVSystem::Views::BatchProcessDialog *m_batchProcessDialog; ///< 批量处理对话框

    // ========== 参数面板控件 ==========

    QWidget *paramWidget;          ///< 参数面板容器
    QScrollArea *paramScrollArea;  ///< 参数滚动区域

    // ========== 自定义控件 ==========

    QFileSystemModel *fileModel;                  ///< 文件系统模型
    ImageFileFilterProxyModel *proxyModel;        ///< 图片文件过滤代理模型
    FileTreeView *treeViewFiles;                  ///< 文件树视图（支持拖放）
    QLabel *labelCurrentPath;                     ///< 当前路径显示标签
    QTabWidget *tabWidget;                        ///< 图片标签页容器
    QTextEdit *textEditLog;                       ///< 日志输出文本框
    QString m_currentBrowsePath;                  ///< 当前浏览的目录路径
    QModelIndex m_contextMenuIndex;               ///< 右键菜单点击的目标索引

    // ========== 图片标签页数据 ==========

    struct TabData {
        QString imagePath;        ///< 图片文件路径
        QPixmap pixmap;           ///< 图片数据
        QStack<QPixmap> undoStack;///< 撤销栈
        QStack<QPixmap> redoStack;///< 重做栈
    };
    QHash<int, TabData> m_tabData;  ///< 标签页数据映射（key为tab索引）

    // ========== 当前活动标签页引用 ==========

    QString m_currentImagePath;   ///< 当前打开的图片路径
    QPixmap m_currentPixmap;      ///< 当前加载的图片数据
    QStack<QPixmap> m_undoStack;  ///< 当前标签页撤销栈
    QStack<QPixmap> m_redoStack;  ///< 当前标签页重做栈
    static const int MAX_UNDO_STEPS = 50;  ///< 最大撤销步数

    /**
     * @brief 保存当前状态到撤销栈
     */
    void saveState();

    /**
     * @brief 更新撤销/重做按钮状态
     */
    void updateUndoRedoState();

    /**
     * @brief 显示保存选项对话框
     * @param previewFileName 副本文件名预览
     * @return 0=覆盖, 1=保存副本, -1=取消
     */
    int showSaveOptionsDialog(const QString &previewFileName);

    /**
     * @brief 生成副本文件路径
     * @param originalPath 原始文件路径
     * @return 副本文件路径
     */
    QString generateCopyFilePath(const QString &originalPath);

    /**
     * @brief 获取已存在的副本编号列表
     * @param baseName 基础文件名
     * @param dir 文件目录
     * @param suffix 文件扩展名
     * @return 已使用的副本编号列表
     */
    QList<int> getExistingCopyNumbers(const QString &baseName,
                                      const QDir &dir,
                                      const QString &suffix);

    // ========== 初始化函数 ==========

    /**
     * @brief 创建并设置所有停靠窗口
     */
    void setupDockWidgets();

    /**
     * @brief 创建主工作区图片展示器
     */
    void setupImageViewer();

    /**
     * @brief 设置文件浏览器（已在 setupDockWidgets 中实现）
     */
    void setupFileBrowser();

    /**
     * @brief 设置日志输出区域（已在 setupDockWidgets 中实现）
     */
    void setupLogOutput();

    /**
     * @brief 设置参数面板（已在 setupDockWidgets 中实现）
     */
    void setupParameterPanel();

    /**
     * @brief 初始化任务菜单
     */
    void setupTaskMenus();

    /**
     * @brief 更新参数面板内容
     * @param task 任务类型
     */
    void updateParameterPanel(CVTask task);

    /**
     * @brief 清空参数面板
     */
    void clearParameterPanel();

    /**
     * @brief 创建图像分类参数面板
     */
    QWidget* createImageClassificationParams();

    /**
     * @brief 创建目标检测参数面板
     */
    QWidget* createObjectDetectionParams();

    /**
     * @brief 创建语义分割参数面板
     */
    QWidget* createSemanticSegmentationParams();

    /**
     * @brief 创建关键点检测参数面板
     */
    QWidget* createKeyPointDetectionParams();

    /**
     * @brief 创建图像增强参数面板
     */
    QWidget* createImageEnhancementParams();

    /**
     * @brief 创建图像去噪参数面板
     */
    QWidget* createImageDenoisingParams();

    /**
     * @brief 创建边缘检测参数面板
     */
    QWidget* createEdgeDetectionParams();

    /**
     * @brief 显示文件右键菜单
     * @param globalPos 菜单显示的全局位置
     * @param filePath 文件路径
     */
    void showFileContextMenu(const QPoint &globalPos, const QString &filePath);

    /**
     * @brief 显示文件夹右键菜单
     * @param globalPos 菜单显示的全局位置
     * @param folderPath 文件夹路径
     */
    void showFolderContextMenu(const QPoint &globalPos, const QString &folderPath);

    /**
     * @brief 显示空白处右键菜单
     * @param globalPos 菜单显示的全局位置
     */
    void showEmptySpaceContextMenu(const QPoint &globalPos);

    // ========== 辅助函数 ==========

    /**
     * @brief 向日志输出区域添加消息
     * @param message 要输出的消息内容
     */
    void logMessage(const QString &message);

    /**
     * @brief 获取当前活动的ImageView
     * @return ImageView指针，如果没有则返回nullptr
     */
    ImageView* currentImageView() const;

    /**
     * @brief 更新当前标签页引用
     */
    void updateCurrentTabRef();

    /**
     * @brief 获取支持的图片格式过滤器字符串
     * @return 图片格式过滤器字符串
     */
    QString getImageFileFilter() const;

    /**
     * @brief 加载并显示图片
     * @param filePath 图片文件路径
     * @return true 成功加载，false 加载失败
     */
    bool loadImage(const QString &filePath);

    /**
     * @brief 关闭当前图片
     */
    void closeImage();

    /**
     * @brief 应用卷积核到图片
     * @param image 输入图片
     * @param kernel 卷积核（3x3）
     * @param divisor 除数
     * @param offset 偏移量
     * @return 处理后的图片
     */
    QImage applyConvolution(const QImage &image, const QVector<QVector<int>> &kernel, int divisor = 1, int offset = 0);

    /**
     * @brief 高斯模糊
     * @param image 输入图片
     * @param radius 模糊半径
     * @return 模糊后的图片
     */
    QImage gaussianBlur(const QImage &image, int radius = 2);

    /**
     * @brief 锐化图片
     * @param image 输入图片
     * @param strength 锐化强度
     * @return 锐化后的图片
     */
    QImage sharpenImage(const QImage &image, double strength = 1.0);

    /**
     * @brief 切换任务类型
     */
    void switchTask(CVTask task);

    // ========== 灰度/反色状态追踪 ==========

    QPixmap m_originalPixmap;       ///< 灰度/反色前的原始图片
    bool m_isGrayscale = false;     ///< 当前是否为灰度状态
    bool m_isInverted = false;      ///< 当前是否为反色状态
};

#endif // MAINWINDOW_H
