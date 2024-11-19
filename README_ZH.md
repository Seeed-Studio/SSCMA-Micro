# SSCMA-Micro CPP SDK

![SSCMA](docs/images/sscma.png)

SSCMA-Micro 是一个为嵌入式设备设计的跨平台机器学习推理框架，服务于 [Seeed Studio SenseCraft Model Assistant](https://github.com/Seeed-Studio/SSCMA)，并提供数字图像处理、神经网络推理、AT 命令交互等功能。

## 概览

### 神经网络

| 目标检测    | 分类                  | 姿态检测        | 异常检测 |
|---------|---------------------|-------------|------|
| FOMO    | IMCLS (MobileNetV2) | PFLD        |      |
| YOLO    |                     | YOLOv8 POSE |      |
| YOLOv8  |                     |             |      |
| YOLOv11 |                     |             |      |

### 模型格式

| TF-Lite | ONNX | 二进制模型 |
|---------|------|------------|
| INT8    |      | CVI 模型    |
| Float32 |      |            |

### 设备支持

| MCUs                                                                                                 | SBCs |
|------------------------------------------------------------------------------------------------------|------|
| Espressif ESP32S3 EYE                                                                                |      |
| [Seeed Studio XIAO (ESP32S3)](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html)                   |      |
| [Seeed Studio Grove Vision AI V1](https://www.seeedstudio.com/Grove-Vision-AI-Module-p-5457.html)     |      |
| [Seeed Studio Grove Vision AI V2](https://www.seeedstudio.com/Grove-Vision-AI-Module-V2-p-5851.html)  |      |

## 许可证

本项目在 [Apache 许可证](LICENSES) 下发布。
