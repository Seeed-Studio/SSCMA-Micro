#ifndef _MA_TYPES_H_
#define _MA_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ma_compiler.h"
#include "ma_config_internal.h"

#ifdef __cplusplus

#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" {
#endif

typedef enum {
    MA_FAILED   = -1,  // UNKOWN ERROR
    MA_OK       = 0,   // No error
    MA_AGAIN    = 1,   // Not ready yet
    MA_ELOG     = 2,   // Logic error
    MA_ETIMEOUT = 3,   // Timeout
    MA_EIO      = 4,   // IO error
    MA_EINVAL   = 5,   // Invalid argument
    MA_ENOMEM   = 6,   // Not enough memory
    MA_EBUSY    = 7,   // Busy
    MA_ENOTSUP  = 8,   // Not supported yet
    MA_EPERM    = 9,   // Operation not permitted
    MA_ENOENT   = 10,  // No such entity
    MA_EEXIST   = 11,  // Entity already exists
    MA_OVERFLOW = 12,  // Overflow
} ma_err_t;

typedef struct {
    void* pool;
    size_t size;
    bool own;
} ma_memory_pool_t;

typedef struct {
    float scale;
    int32_t zero_point;
} ma_quant_param_t;

typedef struct {
    uint32_t size;
    int32_t dims[MA_ENGINE_SHAPE_MAX_DIM];
} ma_shape_t;

typedef enum {
    MA_TENSOR_TYPE_NONE = 0,
    MA_TENSOR_TYPE_U8   = 1,
    MA_TENSOR_TYPE_S8   = 2,
    MA_TENSOR_TYPE_U16  = 3,
    MA_TENSOR_TYPE_S16  = 4,
    MA_TENSOR_TYPE_U32  = 5,
    MA_TENSOR_TYPE_S32  = 6,
    MA_TENSOR_TYPE_U64  = 7,
    MA_TENSOR_TYPE_S64  = 8,
    MA_TENSOR_TYPE_F16  = 9,
    MA_TENSOR_TYPE_F32  = 10,
    MA_TENSOR_TYPE_F64  = 11,
    MA_TENSOR_TYPE_STR  = 12,
    MA_TENSOR_TYPE_BOOL = 13,
    MA_TENSOR_TYPE_BF16 = 14,
    MA_TENSOR_TYPE_NMS_BBOX_U16 = 15,
    MA_TENSOR_TYPE_NMS_BBOX_F32 = 16,
} ma_tensor_type_t;

typedef struct {
    ma_shape_t shape;
    ma_quant_param_t quant_param;
    ma_tensor_type_t type;
    size_t size;
#if MA_USE_ENGINE_TENSOR_INDEX
    size_t index;
#endif
#if MA_USE_ENGINE_TENSOR_NAME
    const char* name;
#endif
    union {
        void* data;
        uint8_t* u8;
        int8_t* s8;
        uint16_t* u16;
        int16_t* s16;
        uint32_t* u32;
        int32_t* s32;
        uint64_t* u64;
        int64_t* s64;
        float* f32;
        double* f64;
    } data;
    bool is_physical;  // For physical tensor
    bool is_variable;  // For constant tensor
    void* external_handler;
} ma_tensor_t;

typedef enum {
    MA_PIXEL_FORMAT_AUTO = 0,
    MA_PIXEL_FORMAT_RGB888,
    MA_PIXEL_FORMAT_RGB565,
    MA_PIXEL_FORMAT_YUV422,
    MA_PIXEL_FORMAT_GRAYSCALE,
    MA_PIXEL_FORMAT_JPEG,
    MA_PIXEL_FORMAT_H264,
    MA_PIXEL_FORMAT_H265,
    MA_PIXEL_FORMAT_RGB888_PLANAR,
    MA_PIXEL_FORMAT_UNKNOWN,
} ma_pixel_format_t;

typedef enum {
    MA_PIXEL_ROTATE_0 = 0,
    MA_PIXEL_ROTATE_90,
    MA_PIXEL_ROTATE_180,
    MA_PIXEL_ROTATE_270,
    MA_PIXEL_ROTATE_UNKNOWN,
} ma_pixel_rotate_t;

typedef struct {
    uint32_t size;
    uint16_t width;
    uint16_t height;
    ma_pixel_format_t format;
    ma_pixel_rotate_t rotate;
    uint32_t timestamp;
    bool key;
    uint8_t index;
    uint8_t count;
    bool physical;  // For physical frame
    uint8_t* data;
} ma_img_t;

typedef struct {
    int64_t preprocess;
    int64_t inference;
    int64_t postprocess;
} ma_perf_t;

typedef struct {
    float x;
    float y;
} ma_pt2f_t;

typedef struct {
    float x;
    float y;
    float z;
} ma_pt3f_t;

typedef struct {
    float x;
    float y;
    float z;
    float t;
} ma_pt4f_t;

typedef struct {
    size_t stride;
    size_t split;
    size_t size;
    size_t start;
} ma_anchor_stride_t;

typedef struct {
    float x;
    float y;
    float score;
    int target;
} ma_point_t;

typedef struct {
    float score;
    int target;
} ma_class_t;

struct ma_bbox_t {
    float x;
    float y;
    float w;
    float h;
    float score;
    int target;
};

#ifdef __cplusplus
struct ma_bbox_ext_t : public ma_bbox_t {
    size_t level;
    size_t index;
};
#endif

struct ma_types {
    /* data */
};

#ifdef __cplusplus
struct ma_keypoint2f_t {
    ma_bbox_t box;
    std::vector<ma_pt2f_t> pts;
};

struct ma_keypoint3f_t {
    ma_bbox_t box;
    std::vector<ma_pt3f_t> pts;
};

struct ma_keypoint4f_t {
    ma_bbox_t box;
    std::vector<ma_pt4f_t> pts;
};

struct ma_segm2f_t {
    ma_bbox_t box;
    struct {
        uint16_t width;
        uint16_t height;
        std::vector<uint8_t> data;
    } mask;
};


#endif

typedef enum {
    MA_MODEL_CFG_OPT_THRESHOLD = 0,
    MA_MODEL_CFG_OPT_NMS       = 1,
    MA_MODEL_CFG_OPT_TOPK      = 2,
} ma_model_cfg_opt_t;

typedef enum {
    MA_TRANSPORT_UNKOWN  = 0,
    MA_TRANSPORT_CONSOLE = 1,
    MA_TRANSPORT_SERIAL  = 2,
    MA_TRANSPORT_SPI     = 3,
    MA_TRANSPORT_I2C     = 4,
    MA_TRANSPORT_MQTT    = 5,
    MA_TRANSPORT_TCP     = 6,
    MA_TRANSPORT_UDP     = 7,
    MA_TRANSPORT_RTSP    = 8,
    MA_TRANSPORT_WS     = 9
} ma_transport_type_t;

typedef enum {
    MA_TRANSPORT_CFG_OPT_NONE    = 0,
    MA_TRANSPORT_CFG_OPT_TIMEOUT = 1,
} ma_transport_cfg_opt_t;

typedef enum { MA_MSG_TYPE_RESP = 0, MA_MSG_TYPE_EVT = 1, MA_MSG_TYPE_LOG = 2, MA_MSG_TYPE_REQ = 3, MA_MSG_TYPE_HB = 4 } ma_msg_type_t;

#define MA_INPUT_TYPE_MASK  0xF000
#define MA_OUTPUT_TYPE_MASK 0x0F00
#define MA_MODEL_TYPE_MASK  0x00FF

typedef enum {
    MA_INPUT_TYPE_TENSOR = 0x0000,
    MA_INPUT_TYPE_IMAGE  = 0x1000,
    MA_INPUT_TYPE_AUDIO  = 0x2000,
} ma_input_type_t;

typedef enum {
    MA_OUTPUT_TYPE_TENSOR       = 0x0000,
    MA_OUTPUT_TYPE_CLASS        = 0x0100,
    MA_OUTPUT_TYPE_POINT        = 0x0200,
    MA_OUTPUT_TYPE_BBOX         = 0x0300,
    MA_OUTPUT_TYPE_KEYPOINT     = 0x0400,
    MA_OUTPUT_TYPE_SEGMENT = 0x0500,
} ma_output_type_t;


typedef enum {
    MA_MODEL_TYPE_UNDEFINED   = 0u,
    MA_MODEL_TYPE_FOMO        = 1u,
    MA_MODEL_TYPE_PFLD        = 2u,
    MA_MODEL_TYPE_YOLOV5      = 3u,
    MA_MODEL_TYPE_IMCLS       = 4u,
    MA_MODEL_TYPE_YOLOV8_POSE = 5u,
    MA_MODEL_TYPE_YOLOV8      = 6u,
    MA_MODEL_TYPE_NVIDIA_DET  = 7u,
    MA_MODEL_TYPE_YOLO_WORLD  = 8u,
    MA_MODEL_TYPE_YOLO11      = 9u,
    MA_MODEL_TYPE_YOLO11_POSE = 10u,
    MA_MODEL_TYPE_YOLO11_SEG  = 11u,
    MA_MODEL_TYPE_YOLOV8_SGE  = 12u,
    MA_MODEL_TYPE_RTMDET      = 13u
} ma_model_type_t;

typedef struct {
    uint8_t id;
    uint32_t size;
    // don't think it a good idea to use self allocated memory in struct
    const void* name;
    const void* addr;
    ma_model_type_t type;
} ma_model_t;

typedef enum { SEC_AUTO = 0, SEC_NONE = 1, SEC_WEP = 2, SEC_WPA1_WPA2 = 3, SEC_WPA2_WPA3 = 4, SEC_WPA3 = 5 } ma_wifi_security_t;

typedef struct {
    char ssid[MA_MAX_WIFI_SSID_LENGTH];
    char bssid[MA_MAX_WIFI_BSSID_LENGTH];
    char password[MA_MAX_WIFI_PASSWORD_LENGTH];
    int8_t security;
} ma_wifi_config_t;

typedef struct {
    char alpn[MA_MQTT_MAX_BROKER_LENGTH];
    char* certification_authority;
    char* client_cert;
    char* client_key;
} ma_mqtt_ssl_config_t;

typedef struct {
    char host[MA_MQTT_MAX_BROKER_LENGTH];
    int16_t port;
    char client_id[MA_MQTT_MAX_CLIENT_ID_LENGTH];
    char username[MA_MQTT_MAX_USERNAME_LENGTH];
    char password[MA_MQTT_MAX_PASSWORD_LENGTH];
    int8_t use_ssl;
} ma_mqtt_config_t;

typedef struct {
    char pub_topic[MA_MQTT_MAX_TOPIC_LENGTH];
    char sub_topic[MA_MQTT_MAX_TOPIC_LENGTH];
    int8_t pub_qos;
    int8_t sub_qos;
} ma_mqtt_topic_config_t;

typedef enum MA_ATTR_PACKED {
    CAM_EVT_OPEN    = 0b00000001,
    CAM_EVT_CLOSE   = 0b00000010,
    CAM_EVT_START   = 0b00000100,
    CAM_EVT_STOP    = 0b00001000,
    CAM_EVT_CAPTURE = 0b00010000,
    CAM_EVT_FRAME   = 0b00100000,
    CAM_EVT_CLEANUP = 0b01000000,
} ma_camera_event_t;

#ifdef __cplusplus
}

#include <string>

struct ipv4_addr_t {
    ipv4_addr_t() : addr{0} {}

    ~ipv4_addr_t() = default;

    uint8_t addr[4];

    // we're not going to validate the input string
    static decltype(auto) from_str(std::string s) {
        ipv4_addr_t r;
        uint8_t l{0};

        for (std::size_t i = 0; i < s.length(); ++i) {
            if (!std::isdigit(s[i]))
                continue;

            uint8_t n{0};
            for (; (++i < s.length()) & (++n < 3);)
                if (!std::isdigit(s[i]))
                    break;
            if (n) {
                auto num{s.substr(i - n, n)};
                r.addr[l++] = static_cast<uint8_t>(std::atoi(num.c_str()));
                if (l > 3)
                    return r;
            }
        }

        return r;
    }

    decltype(auto) to_str() const {
        std::string r{};
        r.reserve(sizeof "255.255.255.255.");
        for (std::size_t i = 0; i < 4; ++i) {
            r += std::to_string(addr[i]);
            r += '.';
        }
        return r.substr(0, r.length() - 1);
    }
};

struct ipv6_addr_t {
    ipv6_addr_t() : addr{0} {}
    ~ipv6_addr_t() = default;

    uint16_t addr[8];

    static decltype(auto) from_str(std::string s) {
        ipv6_addr_t r;
        uint8_t l{0};

        for (std::size_t i = 0; i < s.length(); ++i) {
            if (!std::isxdigit(s[i]))
                continue;

            uint16_t n{0};
            for (; (++i < s.length()) & (++n < 4);)
                if (!std::isxdigit(s[i]))
                    break;
            if (n) {
                auto num{s.substr(i - n, n)};
                r.addr[l++] = static_cast<uint16_t>(std::strtol(num.c_str(), nullptr, 16));
                if (l > 7)
                    return r;
            }
        }

        return r;
    }

    decltype(auto) to_str() const {
        static const char* digits = "0123456789abcdef";
        std::string r;
        r.reserve(sizeof "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff:");
        for (std::size_t i = 0; i < 8; ++i) {
            if (addr[i]) {
                std::string t;
                t.reserve(4);
                for (uint16_t n = addr[i]; n; n >>= 4)
                    t += digits[n & 0x0f];
                r.append(t.rbegin(), t.rend());
            } else
                r += '0';
            r += ':';
        }
        return r.substr(0, r.length() - 1);
    }
};

typedef struct in4_info_t {
    ipv4_addr_t ip;
    ipv4_addr_t netmask;
    ipv4_addr_t gateway;
} in4_info_t;

typedef struct in6_info_t {
    ipv6_addr_t ip;
} in6_info_t;

#endif

#endif  // _MA_TYPES_H_
