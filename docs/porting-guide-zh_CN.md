# SSCMA-Micro 移植指南

SSCMA-Micro 为 SSCMA 准备的将模型部署到 MCUs, SBCs 和其他嵌入式设备的解决方案，它具有小巧的体积和低功耗的特点，可以在资源受限的设备上运行，并且支持多种硬件平台，是统一的模型部署 SDK。该 SDK 整合了多种 SSCMA 中深度学习算法，包括目标检测、人脸识别、姿态估计等。我们为不同的硬件平台提供了示例代码，以便您可以快速部署模型到您的设备上。

## 概述

本指南将指导您如何将 SSCMA-Micro 移植到您的设备上，我们将提供不同层级的移植指南，以满足不同用户的需求。

## 硬件及软件平台要求

- 硬件需求
    - 最少 1 个 CPU 核心
    - 最少 1MB 的 RAM
    - 最少 2MB 的 Flash 存储空间
- 软件需求
    - RTOS 或者 Posix 标准的操作系统
    - 支持 C++11 标准的编译器
    - 支持 CMake 或 Makefile 构建系统

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

*TODO:*

1. 设计统一的插件接口，方便用户扩展功能。

## 快速移植指南

如果您的设备满足硬件及软件平台要求，即支持 C++11 标准的编译器和 CMake 构建系统，并且满足以下条件:

- 支持 FreeRTOS 或者 Posix 标准的操作系统
- 支持 Tensorflow Lite Micro 或者 CVI 推理引擎

您可以按照以下步骤将 SSCMA-Micro 移植到您的设备上:

1. 克隆 SSCMA-Micro 仓库到您的 SDK 目录下
    ```sh
    git clone https://github.com/Seeed-Studio/SSCMA-Micro -d 1 sscma-micro
    ```

2. 熟悉 SSCMA-Micro 的目录结构

3. 在 `sscma-micro` 同级目录下创建 `sscma-micro-porting` 目录，用于存放移植平台的代码，可参考以下目录结构:
    ```sh
    sscma-micro-porting
    ├── CMakeLists.txt
    ├── 3rdparty
    ├── porting
    └── scripts
    ```
    - `CMakeLists.txt`: CMake 构建文件。
    - `3rdparty`: 第三方依赖项。
    - `porting`: 移植代码。
    - `scripts`: 辅助脚本工具目录。
    
4. 关注 `porting` 目录下的结构和文件，这里是移植代码的主要目录，您需要根据您的设备特性实现相关接口。
    ```sh
    sscma-micro-porting/porting
    └── vendor
        └── device
            ├── ma_camera.cpp
            ├── ma_camera.h
            ├── ma_config_board.h
            ├── ma_device.cpp
            ├── ma_misc.c
            ├── ma_storage_lfs.cpp
            ├── ma_storage_lfs.h
            ├── ma_transport_console.cpp
            ├── ma_transport_console.h
            ├── boards
            │   ├── ma_board_A.h
            │   └── ma_board_B.h
            └── drivers
                └── bd
                    └── lfs_flashbd.c
    ```
    - `vendor`: 厂商目录。
    - `device`: 设备目录。
    - `boards`: 板级支持包，包含不同板级的配置文件，比如名称、引脚定义等，如 A、B 两种板级。
    - `drivers`: 驱动程序，包含设备驱动、存储驱动等。
    - `ma_config_board.h`: 板级配置文件，定义板级相关的配置，如具体使用的板级名称、设备 OS 类型，推理引擎等。
    - `ma_device.cpp`: 统一的设备接口实现，上层通过该接口调用设备相关的硬件功能。
    - 其它文件: 根据您的设备特性实现相关接口，比如相机传感器、存储器、传输接口等。

5. 配置 `ma_config_board.h` 文件，定义板级相关的配置，如具体使用的板级名称、设备 OS 类型，推理引擎等。
    需要注意的配置项目有:
    - `MA_BOARD_NAME`: 板级名称，用于区分不同的板级，必须提供，一般使用设备型号，定义为有限长度的 ASCII 字符串。
    - `MA_USE_ENGINE_<ENGINE_NAME>`: 使用的推理引擎，目前默认支持 TFLITE 和 CVI 两种推理引擎，用户可以根据需要选择（其它引擎需参考高级移植指南），定义为 0 或 1。
        - 对于 TFLITE 引擎，需要定义 `MA_ENGINE_TFLITE_TENSOE_ARENA_SIZE`（确定引擎可用的内存字节数）, `MA_USE_STATIC_TENSOR_ARENA`（是否静态分配内存）和 ` MA_TFLITE_OP_<OP_NAME>`（选择支持且需要加入编译的算子）等配置。
        - 对于 CVI 引擎，需要定义 `MA_ENGINE_CVI_TENSOE_ARENA_SIZE`（确定引擎可用的内存字节数）和 `MA_USE_STATIC_TENSOR_ARENA`（是否静态分配内存）等配置。
    - `MA_USE_EXTERNAL_WIFI_STATUS`: 如果设备支持内部或外部的 Wi-Fi 模块，需要定义为 1，否则定义为 0。
        如果定义为 1，则需要实现以下接口，在此 Linker 链接时，上层代码会调用这些接口:
        - `Mutex _net_sync_mutex`: 网络同步锁（在访问以下 POD 接口体变量时，上层会加锁，这也要求在此 porting 中也加锁）。
        - `in4_info_t _in4_info`: IPv4 信息。
        - `in6_info_t _in6_info`: IPv6 信息。
        - `volatile int _net_sta`: 网络状态（0: 断开，1: 可用，2: 已加入网络，3: 已连接相关应用层服务，如 MQTT）。
        
        *TODO:* 使用过 C 标准定义以上接口，并加以 `ma_` 前缀在链接时予以约束。
    - `MA_USE_LIB_<3RD_PARTY_LIB_NAME>`: 使用的第三方库，用户可以根据需要选择，定义为 0 或 1。
        - 对于使用的第三方库，需要定义 `MA_LIB_<3RD_PARTY_LIB_NAME>_VERSION`（版本号）等配置。
        - 目前可用的第三方库有 JPEGENC（用于加速 JPEG 编码）。
    - `MA_INVOKE_ENABLE_RUN_HOOK`: 是否启用推理运行时钩子，一般用于精细控制传感器数据在推理过程中的生命周期，以应对算法抽象的设计瑕疵，并提高推理效率，定义为 1 或 0。
        如果定义为 1，则需要实现以下接口，在此 Linker 链接时，上层代码会调用这些接口:
        - `void ma_invoke_pre_hook(void*)`: 推理前的钩子函数。
        - `void ma_invoke_post_hook(void*)`: 推理后的钩子函数。
    - `MA_FILESYSTEM_<FS_NAME>`: 使用的文件系统，用户可以根据需要选择，定义为 0 或 1。
        - 对于使用的文件系统，需要定义 `MA_FS_<FS_NAME>_VERSION`（版本号）等配置。
        - 目前可用的文件系统有 LITTLEFS，JSON 文件系统等。
        - 如果使用 LITTLEFS，则需要定义 `MA_STORAGE_LFS_USE_<LFS_BD_NAME>` 来确定使用的 Block Device 驱动，如 FLAHSBD。
    - `MA_OSAL_<OS_NAME>`: 使用的操作系统抽象层，用户可以根据需要选择，定义为 0 或 1。
        - 对于使用的操作系统抽象层，需要定义 `MA_OSAL_<OS_NAME>_VERSION`（版本号）等配置。
        - 目前可用的操作系统抽象层有 FreeRTOS，Posix 等。
        - 如果使用 FreeRTOS，则需要定义 `MA_SEVER_AT_EXECUTOR_STACK_SIZE`（AT 服务线程栈大小）和 `MA_SEVER_AT_EXECUTOR_PRIORITY`（AT 服务线程优先级）等配置。

6. 编写 `ma_device.cpp` 文件，实现设备相关的硬件功能接口，如相机传感器、存储器、传输接口等。
    该接口是以单例的形式存在，上层通过该接口调用设备相关的硬件功能，如初始化相机传感器、拍照、存储图片等。
    ```c++
    class Device final {
       public:
        static Device* getInstance() noexcept;
        ~Device();
    
        const std::string& name() const noexcept { return m_name; }
        const std::string& id() const noexcept { return m_id; }
        const std::string& version() const noexcept { return m_version; }
        size_t             bootCount() const noexcept { return m_bootcount; }
    
        const std::vector<Transport*>& getTransports() noexcept { return m_transports; }
        const std::vector<Sensor*>&    getSensors() noexcept { return m_sensors; }
        const std::vector<ma_model_t>& getModels() noexcept { return m_models; }
        Storage*                       getStorage() noexcept { return m_storage; }
    };
    ```
    需要注意的是:
    - Device 所有的设备应在其构造函数中初始化，以确保设备的正常工作。且构造过程中，不允许抛出异常，因此需要在构造函数中处理所有可能的错误。接口 `getInstance` 用于获取 Device 实例，其会被多次调用，因此需要保证其返回的是同一个实例，建议使用过 static 单例，线程安全性由上层编译器参数控制。
    - Trnasport 传输接口，用于设备与外部通信，如串口、Wi-Fi、蓝牙等。在运行时不允许增加或删除 Transport 实例。
    - Sensor 传感器接口，用于设备采集环境数据，如相机、温湿度传感器等。在运行时不允许增加或删除 Sensor 实例。
    - Model 模型接口，用于设备加载和运行模型。在运行时不允许增加或删除 Model 实例。在 `ma_model_t` 结构体中, `name` 和 `addr` 为指针，生命周期在此 Device 内部管理，上层不需要关心其释放。并且在不同的 OS 环境下，`addr` 的类型可能不同，需要根据实际情况进行转换。比如在 FreeRTOS 环境下，`addr` 是对应模型在 Flash 中的地址，而在 Posix 环境下，`addr` 是对应模型在文件系统下，null terminated 的文件路径。
    - Storage 存储接口，用于设备存储数据，默认是一个 KV 存储，用于存储设备的配置信息、模型等。在运行时不允许增加或删除 Storage 实例。
    - 某些情况下，比如使用过 RTOS，可能还需要重载 new/delete 操作符。

7. 编写 `ma_transport_<type>.<cpp/h>`，实现基本的 IO 接口，如 UART、SPI、I2C、MQTT 等。
    ```c++
    class Transport {
       public:
        explicit Transport(ma_transport_type_t type) noexcept : m_initialized(false), m_type(type) {}
        virtual ~Transport() = default;
    
        Transport(const Transport&)            = delete;
        Transport& operator=(const Transport&) = delete;
    
        [[nodiscard]] virtual ma_err_t init(const void* config) noexcept = 0;
        virtual void                   deInit() noexcept                 = 0;
    
        [[nodiscard]] operator bool() const noexcept { return m_initialized; }
        [[nodiscard]] ma_transport_type_t getType() const noexcept { return m_type; }
    
        [[nodiscard]] virtual size_t available() const noexcept                                    = 0;
        virtual size_t               send(const char* data, size_t length) noexcept                = 0;
        virtual size_t               flush() noexcept                                              = 0;
        virtual size_t               receive(char* data, size_t length) noexcept                   = 0;
        virtual size_t               receiveIf(char* data, size_t length, char delimiter) noexcept = 0;
    
       protected:
        bool                m_initialized;
        ma_transport_type_t m_type;
    };
    ```
    需要注意的是:
    - 所有的 Transport 以继承的形式存在，上层通过该接口调用设备相关的传输接口，一般不会使用 `init` 和 `deInit` 接口（一般在此 Device 构造函数中初始化）。
    - `init` 接口用于初始化传输接口，包含一个类型擦除的配置参数，具体配置参数由具体的传输接口决定，调用结果由返回值表示，不允许抛出异常。
    - 构造 Transport 时，需要传入一个 `ma_transport_type_t` 类型的枚举参数，用于区分不同的传输接口。
    - 除了 `flush` 接口外，其它接口可以是阻塞或非阻塞的，具体实现由具体的传输接口决定。
    - 线程安全性在外部不做保证，一般建议在 Transport 内部加锁，以确保线程安全。
    - 至少应该提供一个基础的 Transport 实现，作为 Console Transport，用于设备与外部的基本通信。
    - 所有的 Transport 状态在内部管理，不允许外部修改，对于实际有状态的 Transport，有必要在内部新开一个线程，用于处理状态变化。

8. 编写 `ma_storage_<type>.<cpp/h>`，实现基本的存储接口，如 Flash、SD 卡、文件系统等。
    ```c++
    class Storage {
    public:
        Storage() : m_initialized(false) {}
        virtual ~Storage() = default;
    
        Storage(const Storage&)            = delete;
        Storage& operator=(const Storage&) = delete;
    
        virtual ma_err_t init(const void* config) noexcept = 0;
        virtual void deInit() noexcept                     = 0;
    
        operator bool() const noexcept {
            return m_initialized;
        }
    
        virtual ma_err_t set(const std::string& key, int64_t value) noexcept                  = 0;
        virtual ma_err_t set(const std::string& key, double value) noexcept                   = 0;
        virtual ma_err_t get(const std::string& key, int64_t& value) noexcept                 = 0;
        virtual ma_err_t get(const std::string& key, double& value) noexcept                  = 0;
        virtual ma_err_t set(const std::string& key, const void* value, size_t size) noexcept = 0;
        virtual ma_err_t get(const std::string& key, std::string& value) noexcept             = 0;
    
        virtual ma_err_t remove(const std::string& key) noexcept = 0;
        virtual bool exists(const std::string& key) noexcept     = 0;
    
    protected:
        bool m_initialized;
    };
    ```
    需要注意的是:
    - 所有的 Storage 以继承的形式存在，上层通过该接口调用设备相关的存储接口，一般不会使用 `init` 和 `deInit` 接口（一般在此 Device 构造函数中初始化）。
    - `init` 接口用于初始化存储接口，包含一个类型擦除的配置参数，具体配置参数由具体的存储接口决定，调用结果由返回值表示，不允许抛出异常。
    - 线程安全性在外部不做保证，一般建议在 Storage 内部加锁。
    - 所有上层的操作，`MA_STORAGE_<OP_NAME>` 都可以由该宏定义被重载，以实现不同的存储操作。
    - 对于使用 LITTLEFS 文件系统的存储接口，还需要实现 Block Device 驱动，用于底层的 Flash 存储器的读写操作。
        ```c
        struct lfs_flashbd_config {
            lfs_size_t read_size;
            lfs_size_t prog_size;
            lfs_size_t erase_size;
            lfs_size_t erase_count;
            void*      flash_addr;
        };
        
        typedef struct lfs_flashbd {
            void*                            flash_addr;
            const struct lfs_flashbd_config* cfg;
        } lfs_flashbd_t;

        int lfs_flashbd_create(const struct lfs_config* cfg, const struct lfs_flashbd_config* bdcfg);

        int lfs_flashbd_destroy(const struct lfs_config* cfg);
        
        int lfs_flashbd_read(const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
        
        int lfs_flashbd_prog(const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
        
        int lfs_flashbd_erase(const struct lfs_config* cfg, lfs_block_t block);
        
        int lfs_flashbd_sync(const struct lfs_config* cfg);
        ```
        
    *TODO:* 该接口需要被简化，以减少用户的工作量。


8. 编写 `ma_sensor_<type>.<cpp/h>`，实现基本的传感器接口，如相机、温湿度传感器等。
    ```c++
    class Sensor {
       public:
        enum class Type : int {
            kUnknown    = 0,
            kCamera     = 1,
            kMicrophone = 2,
            kIMU        = 3,
        };
    
        static std::string __repr__(Type type) noexcept {
            switch (type) {
            case Type::kCamera:
                return "Camera";
            case Type::kMicrophone:
                return "Microphone";
            case Type::kIMU:
                return "IMU";
            default:
                return "Unknown";
            }
        }
    
        struct Preset {
            const char* description = nullptr;
        };
    
        using Presets = std::vector<Preset>;
    
       public:
        explicit Sensor(size_t id, Type type) noexcept : m_id(id), m_type(type), m_initialized(false), m_preset_idx(0) {
            if (m_presets.empty()) {
                m_presets.push_back({.description = "Default"});
            }
        }
        virtual ~Sensor() = default;
    
        [[nodiscard]] virtual ma_err_t init(size_t preset_idx) noexcept = 0;
        virtual void                   deInit() noexcept                = 0;
    
        [[nodiscard]] operator bool() const noexcept { return m_initialized; }
    
        [[nodiscard]] size_t getID() const noexcept { return m_id; }
        [[nodiscard]] Type   getType() const noexcept { return m_type; }
    
        const Presets& availablePresets() const noexcept { return m_presets; }
        const Preset&  currentPreset() const noexcept {
            if (m_preset_idx < m_presets.size()) {
                return m_presets[m_preset_idx];
            }
            return m_presets[0];
        }
        const size_t currentPresetIdx() const noexcept { return m_preset_idx; }
    
       protected:
        const size_t m_id;
        const Type   m_type;
        bool         m_initialized;
        size_t       m_preset_idx;
        Presets      m_presets;
    };
    ```
    需要注意的是:
    - 所有的 Sensor 以继承的形式存在，上层通过该接口调用设备相关的传感器接口，一般会使用 `init` 和 `deInit` 接口，不需要在此 Device 构造函数中初始化。
    - `init` 接口用于初始化传感器接口，包含一个预设参数，具体配置参数由具体的传感器接口决定，调用结果由返回值表示，不允许抛出异常。
    - 线程安全性在外部不做保证，一般建议在 Sensor 内部加锁。
    - 所有的 Sensor 状态在内部管理，不允许外部修改，对于实际有状态的 Sensor，有必要在内部新开一个线程，用于处理状态变化。
    - Sensor 一般会有多个预设参数，用于不同的场景，用户可以通过 `init` 接口选择不同的预设参数。
    - 每个 Sensor 都有一个唯一的 ID，用于区分不同的 Sensor，一般由 Device 分配。
    - 在构造 Sensor 时，需要传入一个 `Type` 类型的枚举参数，用于区分不同的传感器接口。

9. 编写 `ma_camera_<type>.<cpp/h>`，实现相机传感器接口。
    ```c++
    class Camera : public Sensor {
       public:
        enum StreamMode : int {
            kUnknown,
            kRefreshOnReturn,
            kRefreshOnRetrieve,
        };
    
        static std::string __repr__(StreamMode mode) noexcept {
            switch (mode) {
            case StreamMode::kRefreshOnReturn:
                return "RefreshOnReturn";
            case StreamMode::kRefreshOnRetrieve:
                return "RefreshOnRetrieve";
            default:
                return "Unknown";
            }
        }
    
        enum CtrlType : int {
            kWindow,
            kExposure,
            kGain,
            kWhiteBalance,
            kFocus,
            kZoom,
            kPan,
            kTilt,
            kIris,
            kShutter,
            kBrightness,
            kContrast,
            kSaturation,
            kHue,
            kSharpness,
            kGamma,
            kColorTemperature,
            kBacklightCompensation,
            kRotate,
            kFormat,
            kChannel,
            kFps,
            kRegister,
        };
    
        enum CtrlMode : int {
            kRead,
            kWrite,
        };
    
        struct CtrlValue {
            union {
                uint8_t  bytes[4];
                uint16_t u16s[2];
                int32_t  i32;
                float    f32;
            };
        };
    
       public:
        Camera(size_t id) noexcept : Sensor(id, Sensor::Type::kCamera), m_streaming(false), m_stream_mode(kUnknown) {}
        virtual ~Camera() = default;
    
        [[nodiscard]] virtual ma_err_t startStream(StreamMode mode) noexcept = 0;
        virtual void                   stopStream() noexcept                 = 0;
    
        [[nodiscard]] virtual ma_err_t commandCtrl(CtrlType ctrl, CtrlMode mode, CtrlValue& value) noexcept = 0;
    
        [[nodiscard]] virtual ma_err_t retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept = 0;
        virtual void                   returnFrame(ma_img_t& frame) noexcept                             = 0;
    
        [[nodiscard]] bool isStreaming() const noexcept { return m_streaming; }
    
       private:
        Camera(const Camera&)            = delete;
        Camera& operator=(const Camera&) = delete;
    
       protected:
        bool       m_streaming;
        StreamMode m_stream_mode;
    };
    ```
    需要注意的是:
    - 所有的 Camera 以继承 Sensor 的形式存在，上层通过该接口调用设备相关的相机传感器接口，一般会使用 `init` 和 `deInit` 接口，不需要在此 Device 构造函数中初始化。
    - `startStream` 接口用于启动相机传感器的视频流，包含一个预设参数，具体配置参数由具体的相机传感器接口决定，调用结果由返回值表示，不允许抛出异常。该参数用于控制视频流的刷新方式，一般有两种模式，一种是在返回帧时刷新，一种是在获取帧时刷新。
    - `stopStream` 接口用于停止相机传感器的视频流，调用结果由返回值表示，不允许抛出异常。
    - `startStream` 和 `stopStream` 接口一般会在 `init` 后和 `deInit` 前被调用，这两个接口一般是成对出现的，之间往往包含很长时间的帧采集过程。 
    - `commandCtrl` 接口用于控制相机传感器的参数，包含一个预设参数，具体配置参数由具体的相机传感器接口决定，调用结果由返回值表示，不允许抛出异常。该参数用于控制相机传感器的参数，如曝光、增益、白平衡、对焦、缩放、旋转等。
    - `retrieveFrame` 接口用于获取相机传感器的帧，包含两个预设参数，具体配置参数由具体的相机传感器接口决定，调用结果由返回值表示，不允许抛出异常。该参数用于获取相机传感器的帧，一般会在 `startStream` 后被调用，获取到的帧数据会被填充到 `ma_img_t` 结构体中。其中第一个参数确定帧的信息，第二个参数确定帧的格式。其中无论是硬件还是软件转码，都需要保证帧的格式正确，并在此 Camera 内部完成转码。
    - `returnFrame` 接口用于返回相机传感器的帧，包含一个预设参数，具体配置参数由具体的相机传感器接口决定，调用结果由返回值表示，不允许抛出异常。该参数用于返回相机传感器的帧，一般会在 `retrieveFrame` 后被调用，返回的帧数据会被释放。
    - 帧的生命周期由 Camera 内部管理，不允许外部修改，对于实际有状态的 Camera，每个被 `retrieveFrame` 获取的帧都需要被 `returnFrame` 返回，否则会导致内存泄漏或无法获取新的帧等异常。
    - 所有的 Camera 状态在内部管理，不允许外部修改，对于实际有状态的 Camera，有必要在内部新开一个线程，用于处理状态变化。
    
    *TODO:* 应使用职能指针，简化传感器数据的生命周期管理。 

11. 编写 `ma_misc.c`，实现一些通用的辅助函数，如日志、时间、内存等。
    ```c
    #ifndef ma_malloc
    void* ma_malloc(size_t size);
    #endif
    
    #ifndef ma_calloc
    void* ma_calloc(size_t nmemb, size_t size);
    #endif
    
    #ifndef ma_realloc
    void* ma_realloc(void* ptr, size_t size);
    #endif
    
    #ifndef ma_free
    void ma_free(void* ptr);
    #endif
    
    #ifndef ma_abort
    void ma_abort(void);
    #endif
    
    #ifndef ma_printf
    int ma_printf(const char* fmt, ...);
    #endif
    
    #ifndef ma_sleep
    void ma_sleep(uint32_t ms);
    #endif
    
    #ifndef ma_usleep
    void ma_usleep(uint32_t us);
    #endif
    
    #ifndef ma_get_time_us
    int64_t ma_get_time_us(void);
    #endif
    
    #ifndef ma_get_time_ms
    int64_t ma_get_time_ms(void);
    #endif
    ```
    需要注意的是:
    - `ma_log` 接口用于打印日志，包含一个格式化字符串和可变参数，具体配置参数由具体的 Misc 接口决定，调用结果由返回值表示，不允许抛出异常。
    - `ma_sleep` 接口用于休眠一段时间，包含一个时间参数，具体配置参数由具体的 Misc 接口决定，调用结果由返回值表示，不允许抛出异常。
    - `ma_malloc` 接口用于分配内存，包含一个大小参数，具体配置参数由具体的 Misc 接口决定，调用结果由返回值表示，不允许抛出异常。
    - `ma_free` 接口用于释放内存，包含一个指针参数，具体配置参数由具体的 Misc 接口决定，调用结果由返回值表示，不允许抛出异常。

12. 整理工程，编写编译脚本，编写测试代码，编译并测试。
    ```c++
    MA_LOGD(MA_TAG, "Initializing Encoder");
    ma::EncoderJSON encoder;

    MA_LOGD(MA_TAG, "Initializing ATServer");
    ma::ATServer server(encoder);

    int ret = 0;

    MA_LOGD(MA_TAG, "Initializing ATServer services");
    ret = server.init();
    if (ret != MA_OK) {
        MA_LOGE(MA_TAG, "ATServer init failed: %d", ret);
    }

    MA_LOGD(MA_TAG, "Starting ATServer");
    ret = server.start();
    if (ret != MA_OK) {
        MA_LOGE(MA_TAG, "ATServer start failed: %d", ret);
    }

    MA_LOGD(MA_TAG, "ATServer started");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ```

至此，您已经完成了 SSCMA-Micro 的移植工作，您可以根据您的设备特性，实现不同的接口，以满足您的需求。

## 高级移植指南

如果您想自定义一些新的算法，以实现更多的功能，以及对于一些特殊的设备，可能需要实现更多的接口，比如使用其它的推理引擎、使用其它的文件系统、使用其它的操作系统等，这些情况下，您需要参考高级移植指南，以实现更多的接口。

### 添加新的算法

如果您想添加新的算法，以实现更多的功能，您可以参考以下步骤:

1. 定位到 `sscma-micro` 目录下的 `core/model` 目录，这里是算法的主要目录，您可以在这里找到所有的算法实现。

2. 在 `core/model` 目录下创建新的文件，用于存放新的算法实现，比如 `core/model/algorithm_<name>.<cpp/h>`。

3. 确定算法的分类，比如分类器、检测器、肢体关键点检测器等，选择一个合适的分类，用于约束输入和输出内容。

4. 根据算法的输入和输出，设计算法的接口，一般包含初始化、推理、后处理等接口。可参考 Model 基类中的纯虚函数定义。
    ```cpp
    class Model {
       private:
        ma_perf_t perf_;
        std::function<void(void*)> p_preprocess_done_     ;
        std::function<void(void*)> p_postprocess_done_    ;
        std::function<void(void*)> p_underlying_run_done_ ;
        void*           p_user_ctx_;
        ma_model_type_t m_type_;
    
       protected:
        Engine*          p_engine_;
        const char*      p_name_;
        virtual ma_err_t preprocess()  = 0;
        virtual ma_err_t postprocess() = 0;
        ma_err_t         underlyingRun();
    
       public:
        Model(Engine* engine, const char* name, ma_model_type_t type);
        virtual ~Model();
        const ma_perf_t  getPerf() const;
        const char*      getName() const;
        ma_model_type_t  getType() const;
        virtual ma_err_t setConfig(ma_model_cfg_opt_t opt, ...) = 0;
        virtual ma_err_t getConfig(ma_model_cfg_opt_t opt, ...) = 0;
        void             setPreprocessDone  (std::function<void(void*)> func);
        void             setPostprocessDone (std::function<void(void*)> func);
        void             setRunDone         (std::function<void(void*)> func);
        void             setUserCtx(void* ctx);
    };
    ```

5. 对新增的算法，增加以下静态函数实现，用于判断算法是否支持、获取算法信息等。
    ```cpp
    static bool isValid(Engine* engine);
    ```
    
6. 在 `ma_model_factory.<h/cpp>` 文件中，注册新增的算法，以便在运行时动态加载。
    注意事项:
    - 某些情况下，需要在 `ma_types.h` 新增数据类型，或者算法的枚举类型，每个算法都有一个唯一的枚举值，用于区分不同的算法。
    - 在上层代码中，可能需要根据算法的枚举值，修改 `refactor_required.hpp` 中的 `serializeAlgorithmOutput` 和 `setAlgorithmInput` 函数，以适配新的算法。
    
    *TODO:* Model 应该被重新设计，避免在上层代码中判断算法是否支持，应该在 Model 内部实现。

7. 测试新的算法，确保算法的正确性和稳定性。


### 添加新的推理引擎

如果您想添加新的推理引擎，以实现更多的功能，您可以参考以下步骤:

1. 定位到 `sscma-micro` 目录下的 `core/engine` 目录，这里是推理引擎的主要目录，您可以在这里找到所有的推理引擎实现。

2. 参考 `ma_engine_base.h` 文件，定义新的推理引擎接口，以非继承的方式实现。
    ```cpp
    class Engine {
    public:
        Engine()          = default;
        virtual ~Engine() = default;
    
        virtual ma_err_t init()                        = 0;
        virtual ma_err_t init(size_t size)             = 0;
        virtual ma_err_t init(void* pool, size_t size) = 0;
    
        virtual ma_err_t run() = 0;
    
        virtual ma_err_t load(const void* model_data, size_t model_size) = 0;
    #if MA_USE_FILESYSTEM
        virtual ma_err_t load(const char* model_path)        = 0;
        virtual ma_err_t load(const std::string& model_path) = 0;
    #endif
    
        virtual int32_t getInputSize()  = 0;
        virtual int32_t getOutputSize() = 0;
    
        virtual ma_tensor_t getInput(int32_t index)                 = 0;
        virtual ma_tensor_t getOutput(int32_t index)                = 0;
        virtual ma_shape_t getInputShape(int32_t index)             = 0;
        virtual ma_shape_t getOutputShape(int32_t index)            = 0;
        virtual ma_quant_param_t getInputQuantParam(int32_t index)  = 0;
        virtual ma_quant_param_t getOutputQuantParam(int32_t index) = 0;
    
        virtual ma_err_t setInput(int32_t index, const ma_tensor_t& tensor) = 0;
    
    #if MA_USE_ENGINE_TENSOR_NAME
        virtual int32_t getInputNum(const char* name)  = 0;
        virtual int32_t getOutputNum(const char* name) = 0;
        virtual ma_tensor_t      getInput(const char* name)            = 0;
        virtual ma_tensor_t      getOutput(const char* name)           = 0;
        virtual ma_shape_t       getInputShape(const char* name)       = 0;
        virtual ma_shape_t       getOutputShape(const char* name)      = 0;
        virtual ma_quant_param_t getInputQuantParam(const char* name)  = 0;
        virtual ma_quant_param_t getOutputQuantParam(const char* name) = 0;
    #endif
    };
    ```
    需要注意的是:
    - 所有的 Engine 以非继承的形式存在，上层通过该接口调用设备相关的推理引擎接口，一般不会使用 `init` 接口（一般在此 Device 构造函数中初始化）。
    - Engine 相关的内存管理和生命周期十分复杂，具体请参考目前已经支持的引擎实现，以确保引擎的正确性和稳定性。


### 添加新的 OS 抽象层

请参考目前已经支持的 OS 抽象层实现，以确保 OS 抽象层的正确性和稳定性。

*Note:* 目前这部分内用有很大概率会被重构，因此在这里不做详细说明。
