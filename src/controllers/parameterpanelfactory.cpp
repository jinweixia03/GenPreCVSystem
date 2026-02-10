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
        case Models::CVTask::InstanceSegmentation:
            return createInstanceSegmentationParams();
        case Models::CVTask::KeyPointDetection:
            return createKeyPointDetectionParams();
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

QWidget* ParameterPanelFactory::createObjectDetectionParams()
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

QWidget* ParameterPanelFactory::createSemanticSegmentationParams()
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

QWidget* ParameterPanelFactory::createInstanceSegmentationParams()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 模型选择
    QGroupBox *modelGroup = new QGroupBox("模型设置", widget);
    QFormLayout *modelLayout = new QFormLayout(modelGroup);
    QComboBox *modelCombo = new QComboBox(widget);
    modelCombo->addItems({"Mask R-CNN", "Cascade Mask R-CNN", "SOLOv2"});
    modelLayout->addRow("模型:", modelCombo);

    QSpinBox *backbone = new QSpinBox(widget);
    backbone->setRange(18, 101);
    backbone->setSingleStep(18);
    backbone->setValue(50);
    modelLayout->addRow("Backbone层数:", backbone);

    QSpinBox *roi = new QSpinBox(widget);
    roi->setRange(32, 1024);
    roi->setValue(256);
    modelLayout->addRow("ROI尺寸:", roi);

    layout->addWidget(modelGroup);

    // 检测参数
    QGroupBox *detectGroup = new QGroupBox("检测参数", widget);
    QFormLayout *detectLayout = new QFormLayout(detectGroup);

    QDoubleSpinBox *confSpinBox = new QDoubleSpinBox(widget);
    confSpinBox->setRange(0.0, 1.0);
    confSpinBox->setDecimals(2);
    confSpinBox->setValue(0.7);
    detectLayout->addRow("置信度阈值:", confSpinBox);

    QSpinBox *minSize = new QSpinBox(widget);
    minSize->setRange(1, 100);
    minSize->setValue(10);
    detectLayout->addRow("最小实例尺寸:", minSize);

    layout->addWidget(detectGroup);

    layout->addStretch();
    return widget;
}

QWidget* ParameterPanelFactory::createKeyPointDetectionParams()
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

QWidget* ParameterPanelFactory::createImageEnhancementParams()
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
    contrastSlider->setRange(-100, 100);
    contrastSlider->setValue(0);
    QLabel *contrastValue = new QLabel("0", widget);
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
    satSlider->setRange(-100, 100);
    satSlider->setValue(0);
    QLabel *satValue = new QLabel("0", widget);
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
    sharpSlider->setRange(0, 100);
    sharpSlider->setValue(0);
    QLabel *sharpValue = new QLabel("0", widget);
    QObject::connect(sharpSlider, &QSlider::valueChanged, [sharpValue](int value) {
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

QWidget* ParameterPanelFactory::createImageDenoisingParams()
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

QWidget* ParameterPanelFactory::createEdgeDetectionParams()
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

} // namespace Controllers
} // namespace GenPreCVSystem
