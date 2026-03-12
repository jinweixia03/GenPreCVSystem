# 项目图片资源

此目录存放 GenPreCVSystem 的图片资源。

## 文件清单

| 文件 | 说明 | 尺寸 |
|------|------|------|
| `logo.svg` | 主 Logo（矢量） | 512x512 |
| `logo-icon.svg` | 图标版 Logo（矢量） | 256x256 |
| `screenshot.png` | 软件截图 | 1200x800+ |

## Logo 说明

Logo 设计寓意：
- **眼睛形状** - 代表计算机视觉（Computer Vision）
- **十字准星** - 代表目标检测和定位
- **网格背景** - 代表像素级图像处理
- **四角标记** - 代表特征点检测和边界框
- **神经网络节点** - 代表深度学习
- **蓝绿色调** - 科技感与 Qt 品牌色呼应

## 使用 SVG Logo

SVG 是矢量格式，可以直接使用：

```html
<!-- 主 Logo -->
<img src="docs/images/logo.svg" width="120">

<!-- 小图标 -->
<img src="docs/images/logo-icon.svg" width="32">
```

## 转换为 PNG（可选）

如需 PNG 格式，可使用以下工具转换：

### 使用 Inkscape（推荐）

```bash
# 512x512 logo
inkscape logo.svg --export-filename=logo.png --export-width=512 --export-height=512

# 256x256 icon
inkscape logo-icon.svg --export-filename=logo-icon.png --export-width=256 --export-height=256

# 32x32 favicon
inkscape logo-icon.svg --export-filename=favicon-32.png --export-width=32 --export-height=32
```

### 使用在线转换器

- [SVG to PNG](https://cloudconvert.com/svg-to-png)
- [Convertio](https://convertio.co/svg-png/)

### 使用 ImageMagick

```bash
convert -background none logo.svg logo.png
```

## 截图指南

### 获取软件截图

1. 启动 GenPreCVSystem
2. 打开一张示例图片
3. 运行一个检测任务（如目标检测）
4. 显示检测结果（带边界框）
5. 截取整个窗口

### 截图要求

- 分辨率：建议 1200x800 或更高
- 格式：PNG（保留透明度）
- 内容：展示主要功能和检测结果
- 图片：使用可自由分发的示例图片

### 推荐示例图片

可以使用以下免费图片资源：
- [Unsplash](https://unsplash.com/) - 免费商用图片
- [Pexels](https://www.pexels.com/) - 免费素材
- [COCO 数据集](https://cocodataset.org/) - 用于测试检测模型

## 品牌色参考

| 颜色 | 色值 | 用途 |
|------|------|------|
| 主蓝色 | `#00d4ff` | Logo 主色 |
| 深青色 | `#0099cc` | 渐变次要色 |
| Qt 绿 | `#41CD52` | 辅助色、角点标记 |
| 深蓝背景 | `#1a1a2e` | 背景色 |
| 次深蓝 | `#16213e` | 渐变背景 |

---

*Logo 由 GenPreCVSystem 团队设计*
