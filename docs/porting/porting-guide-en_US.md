# SSCMA-Micro Porting Guide

SSCMA-Micro is a solution designed for SSCMA to deploy models to MCUs, SBCs, and other embedded devices. It is characterized by its small size and low power consumption, allowing it to operate on devices with limited resources and supporting a variety of hardware platforms. It is a unified model deployment SDK that integrates a variety of deep learning algorithms from SSCMA, including object detection, facial recognition, posture estimation, etc. We provide example code for different hardware platforms so that you can quickly deploy models to your devices.

## Overview

This guide will instruct you on how to port SSCMA-Micro to your device. We will provide porting guides at different levels to meet the needs of different users.

## Hardware and Software Platform Requirements

- Hardware Requirements
    - At least 1 CPU core
    - At least 1MB of RAM
    - At least 2MB of Flash memory
- Software Requirements
    - An RTOS or a Posix-standard operating system
    - A compiler that supports C++11 standard
    - A build system that supports CMake or Makefile

## Directory Structure

The main directory structure is as follows:

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

- `LICENSE`: License file.
- `README.md`: The home page of the project.
- `README_ZH.md`: The Chinese home page of the project.
- `requirements.txt`: The project's dependencies.
- `3rdparty`: Third-party dependencies.
- `docs`: Documentation directory.
- `scripts`: Script directory.
- `sscma`: The source code of SSCMA-Micro.
- `test`: Test directory.

The source code of SSCMA-Micro is located in the `sscma` directory, where you can find all the source code and example code. The following is the structure of the `sscma` directory:

```sh
sscma
├── sscma.h
├── client
├── core
├── extension
├── porting
└── server
```

- `sscma.h`: The header file of SSCMA-Micro.
- `client`: Client code.
- `core`: Core code, including model loading, inference, post-processing, and other functional implementations.
- `extension`: Extension plugin code, including target tracking, target counting, and other extended functional implementations.
- `porting`: Cross-platform related code, including OS software layer and hardware layer interface abstractions.
- `server`: AT service code.

*TODO:*

1. Design a unified plugin interface to facilitate user expansion of functionality.

## Quick Porting Guide

If your device meets the hardware and software platform requirements, i.e., it supports a compiler that complies with the C++11 standard and a CMake build system, and meets the following conditions:

- Supports FreeRTOS or a Posix-standard operating system
- Supports Tensorflow Lite Micro or CVI inference engines

You can follow these steps to port SSCMA-Micro to your device:

1. Clone the SSCMA-Micro repository to your SDK directory
    ```sh
    git clone https://github.com/Seeed-Studio/SSCMA-Micro -d 1 sscma-micro
    ```

2. Familiarize yourself with the directory structure of SSCMA-Micro

3. Create a `sscma-micro-porting` directory at the same level as `sscma-micro` to store the porting platform code. You can refer to the following directory structure:
    ```sh
    sscma-micro-porting
    ├── CMakeLists.txt
    ├── 3rdparty
    ├── porting
    └── scripts
    ```
    - `CMakeLists.txt`: CMake build file.
    - `3rdparty`: Third-party dependencies.
    - `porting`: Porting code.
    - `scripts`: Auxiliary script tool directory.
    
4. Pay attention to the structure and files in the `porting` directory, which is the main directory for porting code. You need to implement the relevant interfaces according to the characteristics of your device.
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
    - `vendor`: Vendor directory.
    - `device`: Device directory.
    - `boards`: Board support package, including configuration files for different board levels, such as names, pin definitions, etc., such as A and B board levels.
    - `drivers`: Driver program, including device drivers, storage drivers, etc.
    - `ma_config_board.h`: Board-level configuration file, defining board-level configurations, such as the specific board name used, device OS type, inference engine, etc.
    - `ma_device.cpp`: Unified device interface implementation, the upper layer calls device-related hardware functions through this interface.
    - Other files: Implement relevant interfaces according to your device characteristics, such as camera sensors, storage devices, transmission interfaces, etc.

5. Configure the `ma_config_board.h` file, defining board-level configurations, such as the specific board name used, device OS type, inference engine, etc.
    The configuration items to note are:
    - `MA_BOARD_NAME`: Board name, used to distinguish between different board levels, must be provided, generally using the device model, defined as a limited-length ASCII string.
    - `MA_USE_ENGINE_<ENGINE_NAME>`: The inference engine used, currently supports TFLITE and CVI inference engines by default, users can choose according to their needs (other engines need to refer to the advanced porting guide), defined as 0 or 1.
        - For the TFLITE engine, you need to define `MA_ENGINE_TFLITE_TENSOE_ARENA_SIZE` (determine the number of bytes of memory available to the engine), `MA_USE_STATIC_TENSOR_ARENA` (whether to statically allocate memory), and `MA_TFLITE_OP_<OP_NAME>` (select the operators that are supported and need to be included in the compilation).
        - For the CVI engine, you need to define `MA_ENGINE_CVI_TENSOE_ARENA_SIZE` (determine the number of bytes of memory available to the engine) and `MA_USE_STATIC_TENSOR_ARENA` (whether to statically allocate memory).
    - `MA_USE_EXTERNAL_WIFI_STATUS`: If the device supports internal or external Wi-Fi modules, it needs to be defined as 1, otherwise defined as 0.
        If defined as 1, the following interfaces need to be implemented, and the upper-layer code will call these interfaces when linking:
        - `Mutex _net_sync_mutex`: Network synchronization lock (when accessing the following POD interface variable, the upper layer will lock, which also requires locking in this porting).
        - `in4_info_t _in4_info`: IPv4 information.
        - `in6_info_t _in6_info`: IPv6 information.
        - `volatile int _net_sta`: Network status (0: disconnected, 1: available, 2: joined the network, 3: connected to related application layer services, such as MQTT).
        
        *TODO:* Define the above interfaces in C standard and add a `ma_` prefix to constrain them when linking.
    - `MA_USE_LIB_<3RD_PARTY_LIB_NAME>`: The third-party library used, users can choose according to their needs, defined as 0 or 1.
        - For the third-party libraries used, you need to define `MA_LIB_<3RD_PARTY_LIB_NAME>_VERSION` (version number) and other configurations.
        - Currently available third-party libraries include JPEGENC (for accelerating JPEG encoding).
    - `MA_INVOKE_ENABLE_RUN_HOOK`: Whether to enable the inference runtime hook, generally used to finely control the lifecycle of sensor data during the inference process to cope with algorithmic abstraction design flaws and improve inference efficiency, defined as 1 or 0.
        If defined as 1, the following interfaces need to be implemented, and the upper-layer code will call these interfaces when linking:
        - `void ma_invoke_pre_hook(void*)`: Pre-inference hook function.
        - `void ma_invoke_post_hook(void*)`: Post-inference hook function.
    - `MA_FILESYSTEM_<FS_NAME>`: The file system used, users can choose according to their needs, defined as 0 or 1.
        - For the file systems used, you need to define `MA_FS_<FS_NAME>_VERSION` (version number) and other configurations.
        - Currently available file systems include LITTLEFS, JSON file systems, etc.
        - If using LITTLEFS, you need to define `MA_STORAGE_LFS_USE_<LFS_BD_NAME>` to determine the Block Device driver used, such as FLASHBD.
    - `MA_OSAL_<OS_NAME>`: The operating system abstraction layer used, users can choose according to their needs, defined as 0 or 1.
        - For the operating system abstraction layers used, you need to define `MA_OSAL_<OS_NAME>_VERSION` (version number) and other configurations.
        - Currently available operating system abstraction layers include FreeRTOS, Posix, etc.
        - If using FreeRTOS, you need to define `MA_SEVER_AT_EXECUTOR_STACK_SIZE` (AT service thread stack size) and `MA_SEVER_AT_EXECUTOR_PRIORITY` (AT service thread priority) and other configurations.

6. Write the `ma_device.cpp` file, implementing device-related hardware function interfaces, such as camera sensors, storage devices, transmission interfaces, etc.
    The interface exists in a singleton form, and the upper layer calls device-related hardware functions through this interface, such as initializing camera sensors, taking photos, storing images, etc.
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
    Notes to consider:
    - All devices in Device should be initialized in its constructor to ensure the normal operation of the device. And the constructor process should handle all possible errors without throwing exceptions. The interface `getInstance` is used to get the Device instance, which will be called multiple times, so it needs to ensure that it returns the same instance, it is recommended to use a static singleton, and thread safety is controlled by the upper-layer compiler parameters.
    - Transport transmission interface, used for device communication with the outside world, such as serial ports, Wi-Fi, Bluetooth, etc. Transport instances should not be added or deleted at runtime.
    - Sensor sensor interface, used for device data collection from the environment, such as cameras, temperature and humidity sensors, etc. Sensor instances should not be added or deleted at runtime.
    - Model model interface, used for device loading and running models. Model instances should not be added or deleted at runtime. In the `ma_model_t` structure, `name` and `addr` are pointers, and their lifecycle is managed internally in Device, the upper layer does not need to care about their release. And in different OS environments, the type of `addr` may be different, it needs to be converted according to the actual situation. For example, in the FreeRTOS environment, `addr` is the address of the model in the Flash, while in the Posix environment, `addr` is the file path of the corresponding model in the file system, null-terminated.
    - Storage storage interface, used for device data storage, the default is a KV storage, used to store device configuration information, models, etc. Storage instances should not be added or deleted at runtime.
    - In some cases, such as using an RTOS, it may also be necessary to override the new/delete operators.

7. Write `ma_transport_<type>.<cpp/h>`, implementing basic IO interfaces, such as UART, SPI, I2C, MQTT, etc.
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
    Notes to consider:
    - All Transports exist in an inherited form, and the upper layer calls device-related transmission interfaces through this interface, generally not using `init` and `deInit` interfaces (generally initialized in the Device constructor).
    - The `init` interface is used to initialize the transmission interface, including a type-erased configuration parameter, the specific configuration parameter is determined by the specific transmission interface, the call result is represented by the return value, and exceptions are not allowed to be thrown.
    - When constructing Transport, you need to pass a `ma_transport_type_t` type enum parameter to distinguish between different transmission interfaces.
    - In addition to the `flush` interface, other interfaces can be blocking or non-blocking, the specific implementation is determined by the specific transmission interface.
    - Thread safety is not guaranteed externally, it is generally recommended to lock inside Transport to ensure thread safety.
    - At least one basic Transport implementation should be provided, as a Console Transport, for basic communication between devices and the outside world.
    - The status of all Transports is managed internally and should not be modified externally, for Transports that actually have status, it is necessary to start a new thread internally to handle status changes.

8. Write `ma_storage_<type>.<cpp/h>`, implementing basic storage interfaces, such as Flash, SD cards, file systems, etc.
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
    Notes to consider:
    - All Storages exist in an inherited form, and the upper layer calls device-related storage interfaces through this interface, generally not using `init` and `deInit` interfaces (generally initialized in the Device constructor).
    - The `init` interface is used to initialize the storage interface, including a type-erased configuration parameter, the specific configuration parameter is determined by the specific storage interface, the call result is represented by the return value, and exceptions are not allowed to be thrown.
    - Thread safety is not guaranteed externally, it is generally recommended to lock inside Storage.
    - All upper-layer operations, `MA_STORAGE_<OP_NAME>` can be overloaded by this macro to implement different storage operations.
    - For storage interfaces using the LITTLEFS file system, it is also necessary to implement the Block Device driver for reading and writing the underlying Flash memory.
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
        
    *TODO:* This interface needs to be simplified to reduce the workload of users.

8. Write `ma_sensor_<type>.<cpp/h>`, implementing basic sensor interfaces, such as cameras, temperature and humidity sensors, etc.
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
    Notes to consider:
    - All Sensors exist in an inherited form, and the upper layer calls device-related sensor interfaces through this interface, generally using `init` and `deInit` interfaces, no need to initialize in the Device constructor.
    - The `init` interface is used to initialize the sensor interface, including a preset parameter, the specific configuration parameter is determined by the specific sensor interface, the call result is represented by the return value, and exceptions are not allowed to be thrown.
    - Thread safety is not guaranteed externally, it is generally recommended to lock inside Sensor.
    - The status of all Sensors is managed internally and should not be modified externally, for Sensors that actually have status, it is necessary to start a new thread internally to handle status changes.
    - Sensors generally have multiple preset parameters for different scenarios, users can choose different preset parameters through the `init` interface.
    - Each Sensor has a unique ID to distinguish different Sensors, generally assigned by Device.
    - When constructing a Sensor, you need to pass a `Type` type enum parameter to distinguish different sensor interfaces.

9. Write `ma_camera_<type>.<cpp/h>`, implementing camera sensor interfaces.
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
    Notes to consider:
    - All Cameras exist in an inherited form from Sensor, and the upper layer calls device-related camera sensor interfaces through this interface, generally using `init` and `deInit` interfaces, no need to initialize in the Device constructor.
    - The `startStream` interface is used to start the video stream of the camera sensor, including a preset parameter, the specific configuration parameter is determined by the specific camera sensor interface, the call result is represented by the return value, and exceptions are not allowed to be thrown. This parameter is used to control the refresh method of the video stream, generally there are two modes, one is to refresh when returning a frame, and the other is to refresh when retrieving a frame.
    - The `stopStream` interface is used to stop the video stream of the camera sensor, the call result is represented by the return value, and exceptions are not allowed to be thrown.
    - The `startStream` and `stopStream` interfaces are generally called after `init` and before `deInit`, these two interfaces are generally paired, and there is often a long process of frame capture between them.
    - The `commandCtrl` interface is used to control the parameters of the camera sensor, including a preset parameter, the specific configuration parameter is determined by the specific camera sensor interface, the call result is represented by the return value, and exceptions are not allowed to be thrown. This parameter is used to control the parameters of the camera sensor, such as exposure, gain, white balance, focus, zoom, rotation, etc.
    - The `retrieveFrame` interface is used to retrieve a frame from the camera sensor, including two preset parameters, the specific configuration parameter is determined by the specific camera sensor interface, the call result is represented by the return value, and exceptions are not allowed to be thrown. This parameter is used to retrieve a frame from the camera sensor, generally called after `startStream`, the retrieved frame data will be filled into the `ma_img_t` structure. Whether it is hardware or software transcoding, it is necessary to ensure the correct format of the frame and complete the transcoding within this Camera.
    - The `returnFrame` interface is used to return a frame to the camera sensor, including a preset parameter, the specific configuration parameter is determined by the specific camera sensor interface, the call result is represented by the return value, and exceptions are not allowed to be thrown. This parameter is used to return a frame to the camera sensor, generally called after `retrieveFrame`, the returned frame data will be released.
    - The lifecycle of the frame is managed internally by the Camera and should not be modified externally. For Cameras that actually have status, each frame obtained by `retrieveFrame` must be returned by `returnFrame`, otherwise it will cause memory leaks or unable to obtain new frames and other exceptions.
    - The status of all Cameras is managed internally and should not be modified externally. For Cameras that actually have status, it is necessary to start a new thread internally to handle status changes.

    *TODO:* Smart pointers should be used to simplify the lifecycle management of sensor data.

11. Write `ma_misc.c`, implementing some common utility functions such as logging, time, memory, etc.
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
    Notes to consider:
    - The `ma_log` interface is used for printing logs, including a formatted string and variable parameters, the specific configuration parameter is determined by the specific Misc interface, the call result is represented by the return value, and exceptions are not allowed to be thrown.
    - The `ma_sleep` interface is used for sleeping for a period of time, including a time parameter, the specific configuration parameter is determined by the specific Misc interface, the call result is represented by the return value, and exceptions are not allowed to be thrown.
    - The `ma_malloc` interface is used for memory allocation, including a size parameter, the specific configuration parameter is determined by the specific Misc interface, the call result is represented by the return value, and exceptions are not allowed to be thrown.
    - The `ma_free` interface is used for memory release, including a pointer parameter, the specific configuration parameter is determined by the specific Misc interface, the call result is represented by the return value, and exceptions are not allowed to be thrown.

12. Organize the project, write the build script, write the test code, compile and test.
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

At this point, you have completed the porting of SSCMA-Micro, and you can implement different interfaces according to the characteristics of your device to meet your needs.

## Advanced Porting Guide

If you want to customize some new algorithms to implement more functions, and for some special devices, you may need to implement more interfaces, such as using other inference engines, using other file systems, using other operating systems, etc., in these cases, you need to refer to the advanced porting guide to implement more interfaces.

### Adding New Algorithms

If you want to add new algorithms to implement more functions, you can refer to the following steps:

1. Locate the `core/model` directory under the `sscma-micro` directory, which is the main directory for algorithms, where you can find all algorithm implementations.

2. Create a new file in the `core/model` directory to store the new algorithm implementation, such as `core/model/algorithm_<name>.<cpp/h>`.

3. Determine the category of the algorithm, such as classifiers, detectors, body keypoint detectors, etc., choose a suitable category to constrain the input and output content.

4. Design the interface of the algorithm based on the input and output, generally including initialization, inference, post-processing, and other interfaces. You can refer to the pure virtual function definition in the Model base class.
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

5. For the added algorithm, add the following static function implementation to determine whether the algorithm is supported and to obtain algorithm information, etc.
    ```cpp
    static bool isValid(Engine* engine);
    ```
    
6. In the `ma_model_factory.<h/cpp>` file, register the added algorithm so that it can be dynamically loaded at runtime.
    Notes:
    - In some cases, you may need to add new data types in `ma_types.h` or algorithm enum types. Each algorithm has a unique enum value to distinguish different algorithms.
    - In the upper-layer code, you may need to modify the `serializeAlgorithmOutput` and `setAlgorithmInput` functions in `refactor_required.hpp` according to the algorithm's enum value to adapt to the new algorithm.
    
    *TODO:* The Model should be redesigned to avoid determining whether the algorithm is supported in the upper-layer code. It should be implemented inside the Model.

7. Test the new algorithm to ensure its correctness and stability.

### Adding New Inference Engines

If you want to add new inference engines to implement more functions, you can refer to the following steps:

1. Locate the `core/engine` directory under the `sscma-micro` directory, which is the main directory for inference engines, where you can find all inference engine implementations.

2. Refer to the `ma_engine_base.h` file and define the new inference engine interface in a non-inherited manner.
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
    Notes to consider:
    - All Engines exist in a non-inherited form, and the upper layer calls device-related inference engine interfaces through this interface, generally not using the `init` interface (generally initialized in the Device constructor).
    - The memory management and lifecycle associated with Engines are very complex, please refer to the implementations of the currently supported engines to ensure the correctness and stability of the engine.

### Adding New OS Abstraction Layers

Please refer to the implementations of the currently supported OS abstraction layers to ensure the correctness and stability of the OS abstraction layer.

*Note:* This part is highly likely to be refactored, so no detailed explanation is provided here.


