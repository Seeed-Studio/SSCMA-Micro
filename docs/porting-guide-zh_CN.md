# SSCMA-Micro 移植指南

## 概述

SSCMA-Micro 为 SSCMA 准备的将模型部署到 MCUs, SBCs 和其他嵌入式设备的解决方案，它具有小巧的体积和低功耗的特点，可以在资源受限的设备上运行，并且支持多种硬件平台，是统一的模型部署 SDK。

该 SDK 整合了多种 SSCMA 中深度学习算法，包括目标检测、人脸识别、姿态估计等。我们为不同的硬件平台提供了示例代码，以便您可以快速部署模型到您的设备上。

本指南将指导您如何将 SSCMA-Micro 移植到您的设备上，我们将提供不同层级的移植指南，以满足不同用户的需求。

## 目录结构

主目录结构如下：

```sh
├── LICENSE
├── README.md
├── README_ZH.md
├── requirements.txt
├── 3rdparty
├── docs
├── scripts
├── sscma
└── test
```
           
- `LICENSE`: 许可证文件。
- `README.md`: 项目的主页。
- `README_ZH.md`: 项目的中文主页。
- `requirements.txt`: 项目的依赖项。
- `3rdparty`: 第三方依赖项。
- `docs`: 文档目录。
- `scripts`: 脚本目录。
- `sscma`: SSCMA-Micro 的源代码。
- `test`: 测试目录。

SSCMA-Micro 的源代码位于 `sscma` 目录下，您可以在这里找到所有的源代码和示例代码，以下是 `sscma` 目录的结构：

```sh
sscma
├── sscma.h
├── client
├── core
├── extension
├── porting
└── server
```

- `sscma.h`: SSCMA-Micro 的头文件。
- `client`: 客户端代码。
- `core`: 核心代码，包含模型加载、推理、后处理等功能实现。
- `extension`: 扩展插件代码，包含如目标跟踪、目标计数能拓展功能实现。
- `porting`: 跨平台相关代码，包含 OS 软件层和硬件层的接口抽象。
- `server`: AT 服务代码。

TODO:

1. 设计统一的插件接口，方便用户扩展功能。

