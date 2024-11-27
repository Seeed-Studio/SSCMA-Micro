# SSCMA-Micro CPP SDK

![SSCMA](docs/images/sscma.png)

SSCMA-Micro is a cross-platform machine learning inference framework designed for embedded devices, serving for [Seeed Studio SenseCraft Model Assistant](https://github.com/Seeed-Studio/SSCMA) and providing functionalities such as digital image processing, neural network inferencing, AT command interaction, and more.


## Overview

### Neural Networks

| Object Detection    | Classification      | Pose Detection | Segmentation | Anomaly Detection |
|---------------------|---------------------|----------------|--------------|-------------------|
| FOMO                | IMCLS (MobileNetV2) | PFLD           | YOLOv11 Seg  |                   |
| Swift YOLO (YOLOv5) | IMCLS (MobileNetV4) | YOLOv8 Pose    |              |                   |
| YOLOv8              |                     | YOLOv11 Pose   |              |                   |
| YOLOv11             |                     |                |              |                   |
| YOLO World          |                     |                |              |                   |
| Nidia Det           |                     |                |              |                   |
| RTMdet             |                     |                |              |                   |

### Model Formats

| TF-Lite | ONNX | Binary Model |
|---------|------|--------------|
| INT8    |      | HEF Model    |
| UINT8   |      | CVI Model    |
| UINT16  |      |              |
| FLOAT32 |      |              |

### Device Support

| MCUs                                                                                                 | SBCs                                                                                           |
|------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|
| Espressif ESP32S3 EYE                                                                                | [Hailo AI (Raspberry Pi)](https://www.seeedstudio.com/Raspberry-Pi-Al-HAT-26-TOPS-p-6243.html) |
| [Seeed Studio XIAO (ESP32S3)](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html)                  |                                                                                                |
| [Seeed Studio Grove Vision AI V1](https://www.seeedstudio.com/Grove-Vision-AI-Module-p-5457.html)    |                                                                                                |
| [Seeed Studio Grove Vision AI V2](https://www.seeedstudio.com/Grove-Vision-AI-Module-V2-p-5851.html) |                                                                                                |
| [Seeed Studio SenseCAP Watcher](https://www.seeedstudio.com/SenseCAP-Watcher-W1-A-p-5979.html)       |                                                                                                |
| [Seeed Studio reCamera](https://www.seeedstudio.com/reCamera-2002-8GB-p-6251.html)                   |                                                                                                |


## License

This project is released under the [Apache License](LICENSES).
