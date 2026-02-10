#ifndef MAINWINDOW_MVC_H
#define MAINWINDOW_MVC_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTextEdit>
#include <QLabel>
#include <QFileSystemModel>
#include <QModelIndex>
#include <QDockWidget>

// Models
#include "models/tasktypes.h"

// Views
#include "views/imageview.h"
#include "views/filetreeview.h"
#include "views/imagefilefilterproxy.h"

// Controllers
#include "controllers/tabcontroller.h"
#include "controllers/imagecontroller.h"
#include "controllers/filecontroller.h"
#include "controllers/taskcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief 主窗口类 (MVC架构重构版)
 *
 * 主窗口作为MVC架构中的View和Controller协调者，负责：
 * - UI组件的组装和布局
 * - 信号槽连接
 * - 协调各Controller之间的交互
 *
 * 业务逻辑已被分离到各个Controller中：
 * - TabController: 标签页管理
 * - ImageController: 图像处理
 * - FileController: 文件操作
 * - TaskController: 任务切换
 */
class MainWindow_MVC : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow_MVC(QWidget *parent = nullptr);
    ~MainWindow_MVC();

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
    void on_actionGrayscale_triggered();
    void on_actionInvert_triggered();
    void on_actionBlur_triggered();
    void on_actionSharpen_triggered();
    void on_actionEdgeDetection_triggered();
    void on_actionThreshold_triggered();

    // ========== 任务菜单槽函数 ==========
    void on_actionTaskImageClassification_triggered();
    void on_actionTaskObjectDetection_triggered();
    void on_actionTaskSemanticSegmentation_triggered();
    void on_actionTaskInstanceSegmentation_triggered();
    void on_actionTaskKeyPointDetection_triggered();
    void on_actionTaskImageEnhancement_triggered();
    void on_actionTaskImageDenoising_triggered();
    void on_actionTaskEdgeDetection_triggered();

    // ========== 工具菜单槽函数 ==========
    void on_actionSettings_triggered();
    void on_actionProcess_triggered();
    void on_actionStop_triggered();

    // ========== 帮助菜单槽函数 ==========
    void on_actionDocumentation_triggered();
    void on_actionShortcuts_triggered();
    void on_actionAbout_triggered();

    // ========== 文件树槽函数 ==========
    void onFileTreeDoubleClicked(const QModelIndex &index);
    void onFileTreeContextMenuRequested(const QPoint &pos);
    void navigateUp();

    // ========== 文件树右键菜单槽函数 ==========
    void onOpenImage();
    void onShowInExplorer();
    void onCopyFilePath();
    void onCopyFile();
    void onDeleteFile();
    void onRenameFile();
    void onOpenFolder();
    void onRefreshFolder();
    void onDeleteFolder();
    void onRefreshView();
    void onCreateNewFolder();
    void onPasteFromFile();

    // ========== 拖放槽函数 ==========
    void onImagesDropped(const QStringList &filePaths);
    void onImageDataDropped(const QByteArray &imageData, const QString &sourceName);
    void onFolderDropped(const QString &folderPath);

    // ========== Controller信号槽 ==========
    void onTabControllerImageLoaded(const QString &filePath, const QPixmap &pixmap);
    void onTabControllerCurrentTabChanged(int index);
    void onImageControllerImageChanged(const QPixmap &newPixmap);
    void onFileControllerLogMessage(const QString &message);
    void onTaskControllerLogMessage(const QString &message);

private:
    void setupUi();
    void setupImageViewer();
    void setupDockWidgets();
    void setupConnections();
    void setupTaskMenus();

    void logMessage(const QString &message);
    void updateUndoRedoState();

    // 文件树右键菜单
    void showFileContextMenu(const QPoint &globalPos, const QString &filePath);
    void showFolderContextMenu(const QPoint &globalPos, const QString &folderPath);
    void showEmptySpaceContextMenu(const QPoint &globalPos);

    // UI组件
    Ui::MainWindow *ui;

    // 停靠窗口
    QDockWidget *dockFileBrowser;
    QDockWidget *dockParameters;
    QDockWidget *dockLogOutput;

    // 自定义控件
    QTabWidget *tabWidget;
    QTextEdit *textEditLog;
    QLabel *labelCurrentPath;

    // 文件系统
    QFileSystemModel *fileModel;
    Views::ImageFileFilterProxyModel *proxyModel;
    Views::FileTreeView *treeViewFiles;
    QString m_currentBrowsePath;
    QModelIndex m_contextMenuIndex;

    // Controllers (MVC架构)
    Controllers::TabController *m_tabController;
    Controllers::ImageController *m_imageController;
    Controllers::FileController *m_fileController;
    Controllers::TaskController *m_taskController;

    // 当前任务
    Models::CVTask m_currentTask;
    QActionGroup *m_taskActionGroup;
};

#endif // MAINWINDOW_MVC_H
