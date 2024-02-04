# SSCMA-Micro CPP SDK

![SSCMA](docs/images/sscma.png)

SSCMA-Micro is a cross-platform machine learning inference framework designed for embedded devices, serving for [Seeed Studio SenseCraft Model Assistant](https://github.com/Seeed-Studio/SSCMA) and providing functionalities such as digital image processing, neural network inferencing, AT command interaction, and more.


## Overview

### Neural Networks

| Object Detection | Classification      | Pose Detection | Anomaly Detection |
|------------------|---------------------|----------------|-------------------|
| FOMO             | IMCLS (MobileNetV2) | PFLD           |                   |
| YOLO             |                     | YOLOv8 Pose    |                   |
| YOLOv8           |                     |                |                   |

### Model Formats

| TF-Lite | ONNX | Binary Model |
|---------|------|--------------|
| INT8    |      |              |

### Device Support

| MCUs                                                                                                 | SBCs |
|------------------------------------------------------------------------------------------------------|------|
| Espressif ESP32S3 EYE                                                                                |      |
| [Seeed Studio XIAO (ESP32S3)](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html)                  |      |
| [Seeed Studio Grove Vision AI V1](https://www.seeedstudio.com/Grove-Vision-AI-Module-p-5457.html)    |      |
| [Seeed Studio Grove Vision AI V2](https://www.seeedstudio.com/Grove-Vision-AI-Module-V2-p-5851.html) |      |


## License

This project is released under the [MIT license](LICENSES).
