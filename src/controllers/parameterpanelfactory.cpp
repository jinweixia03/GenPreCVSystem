#include "parameterpanelfactory.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

namespace GenPreCVSystem {
namespace Controllers {

QWidget* ParameterPanelFactory::createParameterPanel(Models::CVTask task)
{
    switch (task) {
        case Models::CVTask::ImageClassification:
            return createImageClassificationParams();
        case Models::CVTask::ObjectDetection:
            return createObjectDetectionParams();
        case Models::CVTask::SemanticSegmentation:
            return createSemanticSegmentationParams();
        case Models::CVTask::KeyPointDetection:
            return createKeyPointDetectionParams();
        case Models::CVTask::RoadDamageDetection:
            return createRoadDamageDetectionParams();
        case Models::CVTask::ManholeCoverDamageDetection:
            return createManholeCoverDamageDetectionParams();
        case Models::CVTask::RemoteSceneFewShotClassification:
            return createRemoteSceneFewShotClassificationParams();
        case Models::CVTask::ImageEnhancement:
            return createImageEnhancementParams();
        case Models::CVTask::ImageDenoising:
            return createImageDenoisingParams();
        case Models::CVTask::EdgeDetection:
            return createEdgeDetectionParams();
    }
    return nullptr;
}

QWidget* ParameterPanelFactory::createImageClassificationParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("ImageClassificationParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 分类参数
    QGroupBox *paramGroup = new QGroupBox("分类参数", widget);
    QFormLayout *paramLayout = new QFormLayout(paramGroup);

    QSpinBox *topKSpinBox = new QSpinBox(widget);
    topKSpinBox->setObjectName("spinTopK");
    topKSpinBox->setRange(1, 20);
    topKSpinBox->setValue(5);
    paramLayout->addRow("Top-K:", topKSpinBox);

    layout->addWidget(paramGroup);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行分类", widget);
    runBtn->setObjectName("btnRunClassification");
    runBtn->setEnabled(false);
    runBtn->setStyleSheet("QPushButton:disabled { background-color: #cccccc; color: #666666; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createObjectDetectionParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("ObjectDetectionParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 推理参数
    QGroupBox *inferGroup = new QGroupBox("检测参数", widget);
    QFormLayout *inferLayout = new QFormLayout(inferGroup);

    QDoubleSpinBox *confSpinBox = new QDoubleSpinBox(widget);
    confSpinBox->setObjectName("spinConfThreshold");
    confSpinBox->setRange(0.0, 1.0);
    confSpinBox->setDecimals(2);
    confSpinBox->setSingleStep(0.05);
    confSpinBox->setValue(0.25);
    inferLayout->addRow("置信度阈值:", confSpinBox);

    QDoubleSpinBox *iouSpinBox = new QDoubleSpinBox(widget);
    iouSpinBox->setObjectName("spinIOUThreshold");
    iouSpinBox->setRange(0.0, 1.0);
    iouSpinBox->setDecimals(2);
    iouSpinBox->setSingleStep(0.05);
    iouSpinBox->setValue(0.45);
    inferLayout->addRow("IOU阈值:", iouSpinBox);

    QSpinBox *imageSizeSpinBox = new QSpinBox(widget);
    imageSizeSpinBox->setObjectName("spinImageSize");
    imageSizeSpinBox->setRange(320, 1280);
    imageSizeSpinBox->setSingleStep(32);
    imageSizeSpinBox->setValue(640);
    inferLayout->addRow("输入尺寸:", imageSizeSpinBox);

    // 显示选项（仅显示标签）
    QCheckBox *showLabelsCheck = new QCheckBox("显示标签", widget);
    showLabelsCheck->setObjectName("chkShowLabels");
    showLabelsCheck->setChecked(true);
    inferLayout->addRow("显示选项:", showLabelsCheck);

    layout->addWidget(inferGroup);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行检测", widget);
    runBtn->setObjectName("btnRunDetection");
    runBtn->setEnabled(false);
    runBtn->setStyleSheet("QPushButton:disabled { background-color: #cccccc; color: #666666; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createSemanticSegmentationParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("SemanticSegmentationParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 分割参数
    QGroupBox *paramGroup = new QGroupBox("分割参数", widget);
    QFormLayout *paramLayout = new QFormLayout(paramGroup);

    QDoubleSpinBox *confSpinBox = new QDoubleSpinBox(widget);
    confSpinBox->setObjectName("spinConfThreshold");
    confSpinBox->setRange(0.0, 1.0);
    confSpinBox->setDecimals(2);
    confSpinBox->setSingleStep(0.05);
    confSpinBox->setValue(0.25);
    paramLayout->addRow("置信度阈值:", confSpinBox);

    QDoubleSpinBox *iouSpinBox = new QDoubleSpinBox(widget);
    iouSpinBox->setObjectName("spinIOUThreshold");
    iouSpinBox->setRange(0.0, 1.0);
    iouSpinBox->setDecimals(2);
    iouSpinBox->setSingleStep(0.05);
    iouSpinBox->setValue(0.45);
    paramLayout->addRow("IOU阈值:", iouSpinBox);

    QSpinBox *imageSizeSpinBox = new QSpinBox(widget);
    imageSizeSpinBox->setObjectName("spinImageSize");
    imageSizeSpinBox->setRange(320, 1280);
    imageSizeSpinBox->setSingleStep(32);
    imageSizeSpinBox->setValue(640);
    paramLayout->addRow("输入尺寸:", imageSizeSpinBox);

    // 掩码透明度控制
    QSlider *maskAlphaSlider = new QSlider(Qt::Horizontal, widget);
    maskAlphaSlider->setObjectName("sliderMaskAlpha");
    maskAlphaSlider->setRange(10, 90);
    maskAlphaSlider->setValue(50);
    QLabel *maskAlphaValue = new QLabel("50%", widget);
    maskAlphaValue->setObjectName("lblMaskAlpha");
    QObject::connect(maskAlphaSlider, &QSlider::valueChanged, [maskAlphaValue](int value) {
        maskAlphaValue->setText(QString::number(value) + "%");
    });
    QHBoxLayout *maskAlphaLayout = new QHBoxLayout();
    maskAlphaLayout->addWidget(maskAlphaSlider);
    maskAlphaLayout->addWidget(maskAlphaValue);
    paramLayout->addRow("掩码透明度:", maskAlphaLayout);

    // 显示选项（放在同一行）
    QHBoxLayout *displayOptionsLayout = new QHBoxLayout();
    QCheckBox *showBoxesCheck = new QCheckBox("显示边界框", widget);
    showBoxesCheck->setObjectName("chkShowBoxes");
    showBoxesCheck->setChecked(false);  // 默认不显示边界框，只显示蒙版
    displayOptionsLayout->addWidget(showBoxesCheck);

    QCheckBox *showLabelsCheck = new QCheckBox("显示标签", widget);
    showLabelsCheck->setObjectName("chkShowLabels");
    showLabelsCheck->setChecked(true);
    displayOptionsLayout->addWidget(showLabelsCheck);
    displayOptionsLayout->addStretch();

    paramLayout->addRow("显示选项:", displayOptionsLayout);

    layout->addWidget(paramGroup);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行语义分割", widget);
    runBtn->setObjectName("btnRunSegmentation");
    runBtn->setEnabled(false);
    runBtn->setStyleSheet("QPushButton:disabled { background-color: #cccccc; color: #666666; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createKeyPointDetectionParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("KeyPointDetectionParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 推理参数
    QGroupBox *paramGroup = new QGroupBox("关键点检测参数", widget);
    QFormLayout *paramLayout = new QFormLayout(paramGroup);

    QDoubleSpinBox *confSpinBox = new QDoubleSpinBox(widget);
    confSpinBox->setObjectName("spinConfThreshold");
    confSpinBox->setRange(0.0, 1.0);
    confSpinBox->setDecimals(2);
    confSpinBox->setSingleStep(0.05);
    confSpinBox->setValue(0.3);
    paramLayout->addRow("置信度阈值:", confSpinBox);

    QSpinBox *imageSizeSpinBox = new QSpinBox(widget);
    imageSizeSpinBox->setObjectName("spinImageSize");
    imageSizeSpinBox->setRange(320, 1280);
    imageSizeSpinBox->setSingleStep(32);
    imageSizeSpinBox->setValue(640);
    paramLayout->addRow("输入尺寸:", imageSizeSpinBox);

    // 显示选项（放在同一行）
    QHBoxLayout *displayOptionsLayout = new QHBoxLayout();
    QCheckBox *showBoxesCheck = new QCheckBox("显示边界框", widget);
    showBoxesCheck->setObjectName("chkShowBoxes");
    showBoxesCheck->setChecked(true);
    displayOptionsLayout->addWidget(showBoxesCheck);

    QCheckBox *showLabelsCheck = new QCheckBox("显示标签", widget);
    showLabelsCheck->setObjectName("chkShowLabels");
    showLabelsCheck->setChecked(true);
    displayOptionsLayout->addWidget(showLabelsCheck);
    displayOptionsLayout->addStretch();

    paramLayout->addRow("显示选项:", displayOptionsLayout);

    layout->addWidget(paramGroup);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行关键点检测", widget);
    runBtn->setObjectName("btnRunKeyPoint");
    runBtn->setEnabled(false);
    runBtn->setStyleSheet("QPushButton:disabled { background-color: #cccccc; color: #666666; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createRoadDamageDetectionParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("RoadDamageDetectionParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 推理参数
    QGroupBox *inferGroup = new QGroupBox("病害检测参数", widget);
    QFormLayout *inferLayout = new QFormLayout(inferGroup);

    QDoubleSpinBox *confSpinBox = new QDoubleSpinBox(widget);
    confSpinBox->setObjectName("spinConfThreshold");
    confSpinBox->setRange(0.0, 1.0);
    confSpinBox->setDecimals(2);
    confSpinBox->setSingleStep(0.05);
    confSpinBox->setValue(0.25);
    inferLayout->addRow("置信度阈值:", confSpinBox);

    QDoubleSpinBox *iouSpinBox = new QDoubleSpinBox(widget);
    iouSpinBox->setObjectName("spinIOUThreshold");
    iouSpinBox->setRange(0.0, 1.0);
    iouSpinBox->setDecimals(2);
    iouSpinBox->setSingleStep(0.05);
    iouSpinBox->setValue(0.45);
    inferLayout->addRow("IOU阈值:", iouSpinBox);

    QSpinBox *imageSizeSpinBox = new QSpinBox(widget);
    imageSizeSpinBox->setObjectName("spinImageSize");
    imageSizeSpinBox->setRange(320, 1280);
    imageSizeSpinBox->setSingleStep(32);
    imageSizeSpinBox->setValue(640);
    inferLayout->addRow("输入尺寸:", imageSizeSpinBox);

    // 显示选项
    QCheckBox *showLabelsCheck = new QCheckBox("显示标签", widget);
    showLabelsCheck->setObjectName("chkShowLabels");
    showLabelsCheck->setChecked(true);
    inferLayout->addRow("显示选项:", showLabelsCheck);

    layout->addWidget(inferGroup);

    // 类别颜色示意
    QGroupBox *legendGroup = new QGroupBox("病害类别", widget);
    QVBoxLayout *legendLayout = new QVBoxLayout(legendGroup);
    legendLayout->setSpacing(5);

    // 定义道路病害类别和颜色
    struct RoadDamageClass {
        QString color;   // 颜色值如 #FF4444
        QString symbol;  // 符号如 ●
        QString label;   // 标签如 D00 - 纵向裂缝
    };
    RoadDamageClass roadClasses[] = {
        {"#FF4444", "●", "D00 - 纵向裂缝"},
        {"#4444FF", "●", "D10 - 横向裂缝"},
        {"#FFCC00", "●", "D20 - 网状裂缝"},
        {"#AA44FF", "●", "D40 - 坑槽"}
    };

    for (const auto &cls : roadClasses) {
        QLabel *label = new QLabel(widget);
        label->setText(QString("<span style='color:%1; font-weight:bold;'>%2</span> %3")
                       .arg(cls.color, cls.symbol, cls.label));
        label->setStyleSheet("font-size: 12px;");
        legendLayout->addWidget(label);
    }

    layout->addWidget(legendGroup);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行病害检测", widget);
    runBtn->setObjectName("btnRunRoadDamage");
    runBtn->setEnabled(false);
    runBtn->setStyleSheet("QPushButton:disabled { background-color: #cccccc; color: #666666; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createManholeCoverDamageDetectionParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("ManholeCoverDamageDetectionParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 推理参数
    QGroupBox *inferGroup = new QGroupBox("井盖检测参数", widget);
    QFormLayout *inferLayout = new QFormLayout(inferGroup);

    QDoubleSpinBox *confSpinBox = new QDoubleSpinBox(widget);
    confSpinBox->setObjectName("spinConfThreshold");
    confSpinBox->setRange(0.0, 1.0);
    confSpinBox->setDecimals(2);
    confSpinBox->setSingleStep(0.05);
    confSpinBox->setValue(0.25);
    inferLayout->addRow("置信度阈值:", confSpinBox);

    QDoubleSpinBox *iouSpinBox = new QDoubleSpinBox(widget);
    iouSpinBox->setObjectName("spinIOUThreshold");
    iouSpinBox->setRange(0.0, 1.0);
    iouSpinBox->setDecimals(2);
    iouSpinBox->setSingleStep(0.05);
    iouSpinBox->setValue(0.45);
    inferLayout->addRow("IOU阈值:", iouSpinBox);

    QSpinBox *imageSizeSpinBox = new QSpinBox(widget);
    imageSizeSpinBox->setObjectName("spinImageSize");
    imageSizeSpinBox->setRange(320, 1280);
    imageSizeSpinBox->setSingleStep(32);
    imageSizeSpinBox->setValue(640);
    inferLayout->addRow("输入尺寸:", imageSizeSpinBox);

    // 显示选项
    QCheckBox *showLabelsCheck = new QCheckBox("显示标签", widget);
    showLabelsCheck->setObjectName("chkShowLabels");
    showLabelsCheck->setChecked(true);
    inferLayout->addRow("显示选项:", showLabelsCheck);

    layout->addWidget(inferGroup);

    // 类别颜色示意
    QGroupBox *legendGroup = new QGroupBox("井盖状态类别", widget);
    QVBoxLayout *legendLayout = new QVBoxLayout(legendGroup);
    legendLayout->setSpacing(5);

    // 定义井盖病害类别和颜色
    struct ManholeCoverClass {
        QString color;   // 颜色值如 #44AA44
        QString symbol;  // 符号如 ●
        QString label;   // 标签如 井盖 (Manhole)
    };
    ManholeCoverClass coverClasses[] = {
        {"#44AA44", "●", "井盖 (Manhole)"},
        {"#FF8800", "●", "损坏 (Damage)"},
        {"#4444FF", "●", "缺失 (Missing)"},
        {"#FF4444", "●", "打开 (Open)"}
    };

    for (const auto &cls : coverClasses) {
        QLabel *label = new QLabel(widget);
        label->setText(QString("<span style='color:%1; font-weight:bold;'>%2</span> %3")
                       .arg(cls.color, cls.symbol, cls.label));
        label->setStyleSheet("font-size: 12px;");
        legendLayout->addWidget(label);
    }

    layout->addWidget(legendGroup);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行井盖检测", widget);
    runBtn->setObjectName("btnRunManholeCover");
    runBtn->setEnabled(false);
    runBtn->setStyleSheet("QPushButton:disabled { background-color: #cccccc; color: #666666; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createImageEnhancementParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("ImageEnhancementParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 亮度调整
    QGroupBox *brightnessGroup = new QGroupBox("亮度调整", widget);
    QVBoxLayout *brightnessLayout = new QVBoxLayout(brightnessGroup);
    QSlider *brightnessSlider = new QSlider(Qt::Horizontal, widget);
    brightnessSlider->setObjectName("sliderBrightness");
    brightnessSlider->setRange(-100, 100);
    brightnessSlider->setValue(0);
    QLabel *brightnessValue = new QLabel("0", widget);
    brightnessValue->setObjectName("lblBrightness");
    QObject::connect(brightnessSlider, &QSlider::valueChanged, [brightnessValue](int value) {
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
    contrastSlider->setObjectName("sliderContrast");
    contrastSlider->setRange(-100, 100);
    contrastSlider->setValue(0);
    QLabel *contrastValue = new QLabel("0", widget);
    contrastValue->setObjectName("lblContrast");
    QObject::connect(contrastSlider, &QSlider::valueChanged, [contrastValue](int value) {
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
    satSlider->setObjectName("sliderSaturation");
    satSlider->setRange(-100, 100);
    satSlider->setValue(0);
    QLabel *satValue = new QLabel("0", widget);
    satValue->setObjectName("lblSaturation");
    QObject::connect(satSlider, &QSlider::valueChanged, [satValue](int value) {
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
    sharpSlider->setObjectName("sliderSharpness");
    sharpSlider->setRange(0, 100);
    sharpSlider->setValue(0);
    QLabel *sharpValue = new QLabel("0", widget);
    sharpValue->setObjectName("lblSharpness");
    QObject::connect(sharpSlider, &QSlider::valueChanged, [sharpValue](int value) {
        sharpValue->setText(QString::number(value));
    });
    QHBoxLayout *sharpControl = new QHBoxLayout();
    sharpControl->addWidget(sharpSlider);
    sharpControl->addWidget(sharpValue);
    sharpLayout->addLayout(sharpControl);
    layout->addWidget(sharpGroup);

    // 状态标签
    QLabel *statusLabel = new QLabel("状态: 就绪", widget);
    statusLabel->setObjectName("lblEnhanceStatus");
    statusLabel->setStyleSheet("font-weight: bold; color: #666;");
    layout->addWidget(statusLabel);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行增强", widget);
    runBtn->setObjectName("btnRunEnhancement");
    runBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createImageDenoisingParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("ImageDenoisingParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 去噪方法
    QGroupBox *methodGroup = new QGroupBox("去噪方法", widget);
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    QComboBox *methodCombo = new QComboBox(widget);
    methodCombo->setObjectName("cmbDenoiseMethod");
    methodCombo->addItems({"高斯滤波", "双边滤波", "中值滤波", "非局部均值"});
    methodLayout->addWidget(new QLabel("算法:", widget));
    methodLayout->addWidget(methodCombo);
    layout->addWidget(methodGroup);

    // 参数调整
    QGroupBox *paramGroup = new QGroupBox("参数调整", widget);
    QFormLayout *paramLayout = new QFormLayout(paramGroup);

    QSpinBox *kernelSize = new QSpinBox(widget);
    kernelSize->setObjectName("spinKernelSize");
    kernelSize->setRange(1, 15);
    kernelSize->setSingleStep(2);
    kernelSize->setValue(3);
    paramLayout->addRow("卷积核大小:", kernelSize);

    QDoubleSpinBox *sigma = new QDoubleSpinBox(widget);
    sigma->setObjectName("spinSigma");
    sigma->setRange(0.1, 10.0);
    sigma->setDecimals(1);
    sigma->setValue(1.0);
    paramLayout->addRow("Sigma值:", sigma);

    layout->addWidget(paramGroup);

    // 状态标签
    QLabel *statusLabel = new QLabel("状态: 就绪", widget);
    statusLabel->setObjectName("lblDenoiseStatus");
    statusLabel->setStyleSheet("font-weight: bold; color: #666;");
    layout->addWidget(statusLabel);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行去噪", widget);
    runBtn->setObjectName("btnRunDenoising");
    runBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createRemoteSceneFewShotClassificationParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("RemoteSceneFewShotClassificationParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 小样本学习参数
    QGroupBox *fslGroup = new QGroupBox("小样本学习参数", widget);
    QFormLayout *fslLayout = new QFormLayout(fslGroup);

    // N-way: 每轮episode的类别数
    QSpinBox *nWaySpinBox = new QSpinBox(widget);
    nWaySpinBox->setObjectName("spinNWay");
    nWaySpinBox->setRange(2, 20);
    nWaySpinBox->setValue(5);
    nWaySpinBox->setToolTip("每轮任务中的类别数量 (N-way)");
    fslLayout->addRow("N-way (类别数):", nWaySpinBox);

    // N-shot: 每类支持样本数
    QSpinBox *nShotSpinBox = new QSpinBox(widget);
    nShotSpinBox->setObjectName("spinNShot");
    nShotSpinBox->setRange(1, 20);
    nShotSpinBox->setValue(5);
    nShotSpinBox->setToolTip("每个类别的支持样本数量 (N-shot)");
    fslLayout->addRow("N-shot (样本数):", nShotSpinBox);

    // N-query: 每类查询样本数
    QSpinBox *nQuerySpinBox = new QSpinBox(widget);
    nQuerySpinBox->setObjectName("spinNQuery");
    nQuerySpinBox->setRange(1, 50);
    nQuerySpinBox->setValue(15);
    nQuerySpinBox->setToolTip("每个类别的查询样本数量 (N-query)");
    fslLayout->addRow("N-query (查询数):", nQuerySpinBox);

    // 输入图像尺寸
    QSpinBox *imageSizeSpinBox = new QSpinBox(widget);
    imageSizeSpinBox->setObjectName("spinImageSize");
    imageSizeSpinBox->setRange(64, 224);
    imageSizeSpinBox->setSingleStep(16);
    imageSizeSpinBox->setValue(84);
    imageSizeSpinBox->setToolTip("输入图像尺寸 (默认84x84)");
    fslLayout->addRow("输入尺寸:", imageSizeSpinBox);

    layout->addWidget(fslGroup);

    // 简洁的任务说明
    QGroupBox *infoGroup = new QGroupBox("任务简介", widget);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    QLabel *infoLabel = new QLabel(widget);
    infoLabel->setText(
        "<p style='font-size: 11px; color: #666;'>"
        "基于原型网络的小样本学习，通过 N-way-K-shot 方式对遥感影像进行场景分类。"
        "支持 30 类遥感场景（MEET-FSL 数据集）。"
        "</p>"
    );
    infoLabel->setWordWrap(true);
    infoLayout->addWidget(infoLabel);

    // 详细说明按钮
    QPushButton *detailBtn = new QPushButton("📖 查看详细说明...", widget);
    detailBtn->setObjectName("btnFSLDetailInfo");
    detailBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #0066cc; border: 1px solid #0066cc; "
        "padding: 4px 8px; font-size: 11px; }"
        "QPushButton:hover { background-color: #0066cc; color: white; }"
    );
    infoLayout->addWidget(detailBtn);

    layout->addWidget(infoGroup);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行小样本分类", widget);
    runBtn->setObjectName("btnRunFewShotClassification");
    runBtn->setEnabled(false);
    runBtn->setStyleSheet("QPushButton:disabled { background-color: #cccccc; color: #666666; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createEdgeDetectionParams()
{
    QWidget *widget = new QWidget();
    widget->setObjectName("EdgeDetectionParams");
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 检测方法
    QGroupBox *methodGroup = new QGroupBox("检测方法", widget);
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    QComboBox *methodCombo = new QComboBox(widget);
    methodCombo->setObjectName("cmbEdgeMethod");
    methodCombo->addItems({"Sobel", "Canny", "Laplacian", "Scharr"});
    methodLayout->addWidget(new QLabel("算法:", widget));
    methodLayout->addWidget(methodCombo);
    layout->addWidget(methodGroup);

    // Canny参数
    QGroupBox *cannyGroup = new QGroupBox("Canny参数", widget);
    QFormLayout *cannyLayout = new QFormLayout(cannyGroup);

    QDoubleSpinBox *threshold1 = new QDoubleSpinBox(widget);
    threshold1->setObjectName("spinCannyThreshold1");
    threshold1->setRange(0, 500);
    threshold1->setValue(100);
    cannyLayout->addRow("低阈值:", threshold1);

    QDoubleSpinBox *threshold2 = new QDoubleSpinBox(widget);
    threshold2->setObjectName("spinCannyThreshold2");
    threshold2->setRange(0, 500);
    threshold2->setValue(200);
    cannyLayout->addRow("高阈值:", threshold2);

    QSpinBox *aperture = new QSpinBox(widget);
    aperture->setObjectName("spinApertureSize");
    aperture->setRange(3, 7);
    aperture->setSingleStep(2);
    aperture->setValue(3);
    cannyLayout->addRow("孔径大小:", aperture);

    layout->addWidget(cannyGroup);

    // Sobel参数
    QGroupBox *sobelGroup = new QGroupBox("Sobel/Laplacian参数", widget);
    QFormLayout *sobelLayout = new QFormLayout(sobelGroup);

    QSpinBox *ksize = new QSpinBox(widget);
    ksize->setObjectName("spinKernelSize");
    ksize->setRange(1, 7);
    ksize->setSingleStep(2);
    ksize->setValue(3);
    sobelLayout->addRow("卷积核大小:", ksize);

    layout->addWidget(sobelGroup);

    // 状态标签
    QLabel *statusLabel = new QLabel("状态: 就绪", widget);
    statusLabel->setObjectName("lblEdgeStatus");
    statusLabel->setStyleSheet("font-weight: bold; color: #666;");
    layout->addWidget(statusLabel);

    // 执行按钮
    QPushButton *runBtn = new QPushButton("执行边缘检测", widget);
    runBtn->setObjectName("btnRunEdgeDetection");
    runBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    layout->addWidget(runBtn);

    layout->addStretch();
    return widget;
}

} // namespace Controllers
} // namespace GenPreCVSystem
