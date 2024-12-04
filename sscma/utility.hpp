#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <forward_list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/algorithm/el_algorithm_delegate.h"
#include "core/el_types.h"
#include "core/utils/el_base64.h"
#include "core/utils/el_cv.h"
#include "definations.hpp"
#include "porting/el_device.h"
#include "traits.hpp"
#include "types.hpp"

namespace sscma::utility {

using namespace edgelab;
using namespace edgelab::utility;

using namespace sscma::types;
using namespace sscma::traits;

namespace string_concat {

constexpr inline std::size_t lengthof(const char* s) {
    std::size_t size = 0;
    while (*(s + size) ^ '\0') ++size;
    return size;
}

template <class T, std::size_t N> constexpr inline std::size_t lengthof(const T (&)[N]) noexcept { return N; }

inline std::size_t lengthof(const std::string& s) { return s.length(); }

inline std::size_t lengthof(std::string&& s) { return s.length(); }

template <typename... Args> constexpr inline decltype(auto) concat_strings(Args&&... args) {
    std::size_t length{(lengthof(args) + ...)};  // try calculate total length at compile time
    std::string result;
    result.reserve(length + 1);  // preallocate memory, avoid repeatly allocate memory while appendings
    (result.append(std::forward<Args>(args)), ...);
    return result;
}

}  // namespace string_concat

using namespace string_concat;

namespace improc {

inline static uint32_t color_literal(uint8_t i) {
    static const uint16_t color[] = {0x0000, 0x03E0, 0x001F, 0x7FE0, 0xFFFF};
    return color[i % 5];
}

void draw_results_on_image(const std::forward_list<el_point_t>& results, el_img_t* img) {
    uint8_t i = 0;
    for (const auto& point : results) el_draw_point(img, point.x, point.y, color_literal(++i));
}

void draw_results_on_image(const std::forward_list<el_box_t>& results, el_img_t* img) {
    uint8_t i = 0;
    for (const auto& box : results) {
        int16_t y = box.y - (box.h >> 1);  // center y
        int16_t x = box.x - (box.w >> 1);  // center x
        el_draw_rect(img, x, y, box.w, box.h, color_literal(++i), 4);
    }
}

}  // namespace improc

using namespace improc;

decltype(auto) quoted(const std::string& str, const char delim = '"') {
    std::size_t sz = 0;
    for (char c : str) sz += !(c ^ delim) | !(c ^ '\\');
    std::string ss(1, delim);
    ss.reserve(str.length() + (sz << 1) + 2);
    for (char c : str) {
        if (!(c ^ delim) | !(c ^ '\\')) [[unlikely]]
            ss += '\\';
        ss += c;
    }
    ss += delim;
    return ss;
}

template <typename T> decltype(auto) to_hex_string(T dec) {
    static_assert(std::is_unsigned<T>::value);
    static const char* digits = "0123456789abcdef";
    std::size_t const  len    = sizeof(T) << 1;
    if (dec == 0U) return std::string{len, '0'};
    std::size_t const bits = (len - 1) << 2;
    std::string       hex(len, '0');
    for (std::size_t i = 0, j = bits; i < len; ++i, j -= 4) hex[i] = digits[(dec >> j) & 0x0f];
    return hex;
}

decltype(auto) model_info_2_json_str(const el_model_info_t& model_info) {
    return concat_strings("{\"id\": ",
                          std::to_string(model_info.id),
                          ", \"type\": ",
                          std::to_string(model_info.type),
                          ", \"address\": ",
                          std::to_string(model_info.addr_flash),
                          ", \"size\": ",
                          std::to_string(model_info.size),
                          "}");
}

decltype(auto) sensor_info_2_json_str(const el_sensor_info_t& sensor_info, Device* device, bool all_opts = false) {
    std::string opts;

    switch (sensor_info.type) {
    case EL_SENSOR_TYPE_CAM: {
        auto camera = device->get_camera();  // currently only support single camera
        auto opt_id = camera->current_opt_id();
        opts += concat_strings(
          "\"opt_id\": ", std::to_string(opt_id), ", \"opt_detail\": \"", camera->current_opt_detail(), "\"");
        const char* delim = "";
        if (all_opts) {
            opts += ", \"opts\": {";
            for (const auto& opt : camera->supported_opts()) {
                opts += concat_strings(delim, quoted(std::to_string(opt.id)), ": ", quoted(opt.details));
                delim = ", ";
            }
            opts += "}";
        }
    } break;

    default:
        opts = "\"opt_id\": -1, \"opt_detail\": \"Unknown\", \"opts\": {}";
    }

    return concat_strings("{\"id\": ",
                          std::to_string(sensor_info.id),
                          ", \"type\": ",
                          std::to_string(sensor_info.type),
                          ", \"state\": ",
                          std::to_string(sensor_info.state),
                          ", ",
                          std::move(opts),
                          "}");
}

decltype(auto) results_2_json_str(const std::forward_list<el_box_t>& results) {
    std::string ss;
    const char* delim = "";

    ss = "\"boxes\": [";
    for (const auto& box : results) {
        ss += concat_strings(delim,
                             "[",
                             std::to_string(box.x),
                             ", ",
                             std::to_string(box.y),
                             ", ",
                             std::to_string(box.w),
                             ", ",
                             std::to_string(box.h),
                             ", ",
                             std::to_string(box.score),
                             ", ",
                             std::to_string(box.target),
                             "]");
        delim = ", ";
    }
    ss += "]";

    return ss;
}

decltype(auto) results_2_json_str(const std::forward_list<el_point_t>& results) {
    std::string ss;
    const char* delim = "";

    ss = "\"points\": [";
    for (const auto& point : results) {
        ss += concat_strings(delim,
                             "[",
                             std::to_string(point.x),
                             ", ",
                             std::to_string(point.y),
                             ", ",
                             std::to_string(point.score),
                             ", ",
                             std::to_string(point.target),
                             "]");
        delim = ", ";
    }
    ss += "]";

    return ss;
}

decltype(auto) results_2_json_str(const std::forward_list<el_class_t>& results) {
    std::string ss;
    const char* delim = "";

    ss = "\"classes\": [";
    for (const auto& cls : results) {
        ss += concat_strings(delim, "[", std::to_string(cls.score), ", ", std::to_string(cls.target), "]");
        delim = ", ";
    }
    ss += "]";

    return ss;
}

decltype(auto) results_2_json_str(const std::forward_list<el_keypoint_t>& results) {
    std::string ss;
    const char* delim = "";

    ss = "\"keypoints\": [";
    for (const auto& kp : results) {
        std::string pts_str{"["};
        const char* pts_delim = "";
        for (const auto& pt : kp.pts) {
            pts_str += concat_strings(pts_delim,
                                      "[",
                                      std::to_string(pt.x),
                                      ", ",
                                      std::to_string(pt.y),
                                      ", ",
                                      std::to_string(pt.score),
                                      ", ",
                                      std::to_string(pt.target),
                                      "]");
            pts_delim = ", ";
        }
        pts_str += "]";
        ss += concat_strings(delim,
                             "[",
                             "[",
                             std::to_string(kp.box.x),
                             ", ",
                             std::to_string(kp.box.y),
                             ", ",
                             std::to_string(kp.box.w),
                             ", ",
                             std::to_string(kp.box.h),
                             ", ",
                             std::to_string(kp.box.score),
                             ", ",
                             std::to_string(kp.box.target),
                             "]",
                             ", ",
                             std::move(pts_str),
                             "]");
        delim = ", ";
    }
    ss += "]";

    return ss;
}

decltype(auto) img_res_2_json_str(const el_img_t* img) {
    return concat_strings("\"resolution\": [", std::to_string(img->width), ", ", std::to_string(img->height), "]");
}

char* _internal_jpeg_buffer = nullptr;
size_t _internal_jpeg_buffer_size = 0;

decltype(auto) img_2_json_str(const el_img_t* img) {
    if (!img || !img->data || !img->size) [[unlikely]]
        return std::string("\"image\": \"\"");

#if SSCMA_SHARED_BASE64_BUFFER
    static char*       buffer      = reinterpret_cast<char*>(SSCMA_SHARED_BASE64_BUFFER_BASE);
    static std::size_t buffer_size = SSCMA_SHARED_BASE64_BUFFER_SIZE;

    if ((((img->size + 2u) / 3u) << 2u) + 1u > buffer_size) {
        EL_LOGW("Error: shared base64 buffer exhausted");
        return std::string{"\"image\": \"\""};
    }
#else
    static char*       buffer      = nullptr;
    static std::size_t buffer_size = 0;
    static std::size_t size        = 0;

    // only reallcate memory when buffer size is not enough
    if (img->size > size) [[unlikely]] {
        size        = img->size;
        buffer_size = (((size + 2u) / 3u) << 2u) + 1u;  // base64 encoded size, +1 for terminating null character

        if (buffer) [[likely]]
            delete[] buffer;
        buffer = new char[buffer_size]{};
    }
#endif

    std::memset(buffer, 0, buffer_size);
    el_base64_encode(img->data, img->size, buffer);

    int rotate = (360 - (static_cast<int>(img->rotate) * 90)) % 360;
    _internal_jpeg_buffer = buffer;
    _internal_jpeg_buffer_size = buffer_size > 1 ? buffer_size - 1 : 0;
    return concat_strings("\"image\": \"", "\"", ", \"rotate\": ", std::to_string(rotate));
}

// TODO: avoid repeatly allocate/release memory in for loop
decltype(auto) img_2_jpeg_json_str(const el_img_t* img, el_img_t* cache = nullptr) {
    if (!img || !img->data || !img->size) [[unlikely]]
        return std::string("\"image\": \"\"");

    static std::size_t size      = 0;
    static uint8_t*    jpeg_data = nullptr;
    std::size_t        jpeg_size = img->size >> 2u;  // assuing jpeg size is 1/4 of raw image size
    // only reallcate memory when buffer size is not enough (TODO: reallocate logging for profiling)
    if (jpeg_size > size) [[unlikely]] {
        size = jpeg_size;
        if (jpeg_data) [[likely]]
            delete[] jpeg_data;
        jpeg_data = new uint8_t[size]{};
    }

    std::memset(jpeg_data, 0, size);
    auto jpeg_img = el_img_t{.data   = jpeg_data,
                             .size   = size,
                             .width  = img->width,
                             .height = img->height,
                             .format = EL_PIXEL_FORMAT_JPEG,
                             .rotate = img->rotate};

    if (el_img_convert(img, &jpeg_img) == EL_OK) [[likely]] {
        if (cache) [[likely]]
            *cache = jpeg_img;
        return img_2_json_str(&jpeg_img);
    }

    return std::string("\"image\": \"\"");
}

decltype(auto) algorithm_info_2_json_str(const el_algorithm_info_t* info) {
    return concat_strings("{\"type\": ",
                          std::to_string(info->type),
                          ", \"categroy\": ",
                          std::to_string(info->categroy),
                          ", \"input_from\": ",
                          std::to_string(info->input_from),
                          "}");
}

template <typename AlgorithmConfigType>
constexpr decltype(auto) algorithm_config_2_json_str(const AlgorithmConfigType& config) {
    bool        comma{false};
    std::string ss{concat_strings("{\"type\": ",
                                  std::to_string(config.info.type),
                                  ", \"categroy\": ",
                                  std::to_string(config.info.categroy),
                                  ", \"input_from\": ",
                                  std::to_string(config.info.input_from),
                                  ", \"config\": {")};
    if constexpr (has_member_score_threshold<typename std::remove_reference<decltype(config)>::type>()) {
        ss += concat_strings("\"tscore\": ", std::to_string(config.score_threshold));
        comma = true;
    }
    if constexpr (has_member_iou_threshold<typename std::remove_reference<decltype(config)>::type>()) {
        if (comma) ss += ", ";
        ss += concat_strings("\"tiou\": ", std::to_string(config.iou_threshold));
        comma = true;
    }
    ss += "}}";

    return ss;
}

template <typename AlgorithmType> decltype(auto) algorithm_config_2_json_str(std::shared_ptr<AlgorithmType> algorithm) {
    return algorithm_config_2_json_str(algorithm->get_algorithm_config());
}

template <typename AlgorithmType>
decltype(auto) algorithm_results_2_json_str(std::shared_ptr<AlgorithmType> algorithm) {
    std::string ss{concat_strings("\"perf\": [",
                                  std::to_string(algorithm->get_preprocess_time()),
                                  ", ",
                                  std::to_string(algorithm->get_run_time()),
                                  ", ",
                                  std::to_string(algorithm->get_postprocess_time()),
                                  "], ",
                                  results_2_json_str(algorithm->get_results()))};

    return ss;
}

decltype(auto) wifi_config_2_json_str(const wifi_sta_cfg_t& config, bool secure = true) {
    auto        pwd = /* secure ? std::string(std::strlen(config.passwd), '*') : */ std::string(config.passwd);
    std::string ss{concat_strings("{\"name_type\": ",
                                  std::to_string(config.name_type),
                                  ", \"name\": ",
                                  quoted(config.name),
                                  ", \"security\": ",
                                  std::to_string(config.security_type),
                                  ", \"password\": ",
                                  quoted(pwd),
                                  "}")};
    return ss;
}

decltype(auto) mqtt_server_config_2_json_str(const mqtt_server_config_t& config, bool secure = true) {
    auto        pwd = /* secure ? std::string(std::strlen(config.password), '*') : */ std::string(config.password);
    std::string ss{concat_strings("{\"client_id\": ",
                                  quoted(config.client_id),
                                  ", \"address\": ",
                                  quoted(config.address),
                                  ", \"port\": ",
                                  std::to_string(config.port),
                                  ", \"username\": ",
                                  quoted(config.username),
                                  ", \"password\": ",
                                  quoted(pwd),
                                  ", \"use_ssl\": ",
                                  std::to_string(config.use_ssl ? 1 : 0),
                                  "}")};
    return ss;
}

decltype(auto) mqtt_pubsub_config_2_json_str(const mqtt_pubsub_config_t& config) {
    std::string ss{concat_strings("{\"pub_topic\": ",
                                  quoted(config.pub_topic),
                                  ", \"pub_qos\": ",
                                  std::to_string(config.pub_qos),
                                  ", \"sub_topic\": ",
                                  quoted(config.sub_topic),
                                  ", \"sub_qos\": ",
                                  std::to_string(config.sub_qos),
                                  "}")};
    return ss;
}

decltype(auto) tokenize_function_2_argv(const std::string& input) {
    std::vector<std::string> argv;

    std::size_t index = 0;
    std::size_t size  = input.size();
    char        c     = {};

    while (index < size) {                             // tokenize input string
        c = input.at(index);                           // update current character
        if (std::isalnum(c) || c == '_') [[likely]] {  // if current character is alphanumeric or underscore
            size_t prev = index;                       // mark the start of the token
            while (++index < size) {                   // find the end of the token
                c = input.at(index);
                if (!(std::isalnum(c) || c == '_')) [[unlikely]]
                    break;  // if current character is not alphanumeric or underscore, break e.g. '(', ')', ',', ' ' ...
            }
            argv.push_back(input.substr(prev, index - prev));  // push the token into argv
        } else
            ++index;  // if current character is not alphanumeric or underscore, skip it
    }
    argv.shrink_to_fit();

    return argv;
}

bool is_bssid(const std::string& str) {
    if (str.length() != 17) return false;  // BSSID length is 17, e.g. 00:11:22:33:44:55

    for (std::size_t i = 0; i < str.length(); ++i) {
        if (i % 3 == 2) {
            if (str[i] != ':' && str[i] != '-') return false;  // BSSID delimiter is ':' or '-'
        } else {
            if (!std::isxdigit(str[i])) return false;  // BSSID is hex string
        }
    }

    return true;
}

decltype(auto) get_default_mqtt_server_config(const Device* device) {
    auto default_config = mqtt_server_config_t{};
    auto device_id      = to_hex_string(device->get_device_id());

    std::snprintf(default_config.client_id,
                  sizeof(default_config.client_id) - 1,
                  SSCMA_MQTT_CLIENT_ID_FMT,
                  PRODUCT_NAME_PREFIX,
                  PRODUCT_NAME_SUFFIX,
                  device_id.c_str());

    return default_config;
}

decltype(auto) in4_info_2_json_str(const in4_info_t& config) {
    return concat_strings("{\"ip\": \"",
                          config.ip.to_str(),
                          "\", \"netmask\": \"",
                          config.netmask.to_str(),
                          "\", \"gateway\": \"",
                          config.gateway.to_str(),
                          "\"}");
}

decltype(auto) in6_info_2_json_str(const in6_info_t& config) {
    return concat_strings("{\"ip\": \"", config.ip.to_str(), "\"}");
}

}  // namespace sscma::utility
