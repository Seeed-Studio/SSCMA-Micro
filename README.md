# SSCMA-Micro CPP SDK

![SSCMA](docs/images/sscma.png)

SSCMA-Micro is a cross-platform machine learning inference framework designed for embedded devices, serving for [Seeed Studio SenseCraft Model Assistant](https://github.com/Seeed-Studio/SSCMA) and providing functionalities such as digital image processing, neural network inferencing, AT command interaction, and more.


## Overview

### Neural Networks

| Object Detection | Classification      | Pose Detection | Anomaly Detection |
|------------------|---------------------|----------------|-------------------|
| FOMO             | IMCLS (MobileNetV2) | PFLD           |                   |
| YOLO             |                     |                |                   |

### Model Formats

| TF-Lite | ONNX |
|---------|------|
| INT8    |      |

### Device Support

| MCUs                        | SBCs |
|-----------------------------|------|
| Espressif ESP32S3 EYE       |      |
| Seeed Studio XIAO (ESP32S3) |      |
| Himax WE-I                  |      |


## Structure

```
sscma-micro
├── core
│   ├── algorithm
│   ├── data
│   ├── engine
│   ├── synchronize
│   └── utils
├── docs
│   └── images
├── porting
├── sscma
│   ├── callback
│   │   └── internal
│   ├── interpreter
│   └── repl
└── third_party
    ├── FlashDB
    └── JPEGENC
```


## License

This project is released under the [MIT license](LICENSES).
