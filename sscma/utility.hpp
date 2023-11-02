#pragma once

#include <cstdint>
#include <forward_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/algorithm/el_algorithm_delegate.h"
#include "core/data/el_data_storage.hpp"
#include "core/el_types.h"
#include "core/utils/el_base64.h"
#include "core/utils/el_cv.h"
#include "sscma/definations.hpp"
#include "sscma/traits.hpp"

namespace sscma::utility {

using namespace edgelab;
using namespace edgelab::utility;
using namespace sscma::traits;

namespace string_concat {

constexpr inline std::size_t lengthof(const char* s) {
    std::size_t size = 0;
    while (*(s + size) != '\0') ++size;
    return size;
}

template <class T, std::size_t N> constexpr inline std::size_t lengthof(const T (&)[N]) noexcept { return N; }

inline std::size_t lengthof(const std::string& s) { return s.length(); }

template <typename... Args> constexpr inline decltype(auto) concat_strings(Args&&... args) {
    std::size_t length{(lengthof(args) + ...)};
    std::string result;
    result.reserve(length);
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
        int16_t y = box.y - (box.h >> 1);
        int16_t x = box.x - (box.w >> 1);
        el_draw_rect(img, x, y, box.w, box.h, color_literal(++i), 4);
    }
}

}  // namespace improc

using namespace improc;

inline std::string quoted(const std::string& str, const char delim = '"') {
    std::size_t sz = 0;
    for (char c : str)
        if (c == delim) [[unlikely]]
            ++sz;
    std::string ss(1, delim);
    ss.reserve(str.length() + (sz << 1));
    for (char c : str) {
        if (c == delim) [[unlikely]]
            ss += '\\';
        if (c != '\n') [[likely]]
            ss += c;
    }
    ss += delim;
    return ss;
}

std::string model_info_2_json_str(el_model_info_t model_info) {
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

std::string sensor_info_2_json_str(el_sensor_info_t sensor_info) {
    return concat_strings("{\"id\": ",
                          std::to_string(sensor_info.id),
                          ", \"type\": ",
                          std::to_string(sensor_info.type),
                          ", \"state\": ",
                          std::to_string(sensor_info.state),
                          "}");
}

template <typename T> constexpr std::string results_2_json_str(const std::forward_list<T>& results) {
    std::string ss;
    const char* delim = "";

    if constexpr (std::is_same<T, el_box_t>::value) {
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
    } else if constexpr (std::is_same<T, el_point_t>::value) {
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
    } else if constexpr (std::is_same<T, el_class_t>::value) {
        ss = "\"classes\": [";
        for (const auto& cls : results) {
            ss += concat_strings(delim, "[", std::to_string(cls.score), ", ", std::to_string(cls.target), "]");
            delim = ", ";
        }
    }
    ss += "]";

    return ss;
}

// TODO: support reallocate memory when image size (width or height) changes
std::string img_2_json_str(const el_img_t* img) {
    if (!img || !img->data) [[unlikely]]
        return "\"image\": \"\"";

    static std::size_t size        = img->width * img->height * 3;
    static std::size_t buffer_size = ((size + 2) / 3) * 4 + 1;
    static char*       buffer      = new char[buffer_size]{};
    std::memset(buffer, 0, buffer_size);
    el_base64_encode(img->data, img->size, buffer);

    return concat_strings("\"image\": \"", buffer, "\"");
}

// TODO: avoid repeatly allocate/release memory in for loop
std::string img_2_jpeg_json_str(const el_img_t* img) {
    if (!img || !img->data) [[unlikely]]
        return "\"image\": \"\"";

    static std::size_t size      = img->width * img->height * 3;
    static uint8_t*    jpeg_data = new uint8_t[size]{};
    if (img->width * img->height * 3 != size) [[unlikely]] {
        size = img->width * img->height * 3;
        delete[] jpeg_data;
        jpeg_data = new uint8_t[size]{};
    }
    auto jpeg_img = el_img_t{.data   = jpeg_data,
                             .size   = size,
                             .width  = img->width,
                             .height = img->height,
                             .format = EL_PIXEL_FORMAT_JPEG,
                             .rotate = img->rotate};
    std::memset(jpeg_data, 0, size);
    if (el_img_convert(img, &jpeg_img) == EL_OK) [[likely]] {
        // allocate static buffer using maxium size
        static std::size_t buffer_size = ((size + 2) / 3) * 4 + 1;
        static char*       buffer      = new char[buffer_size]{};
        std::memset(buffer, 0, buffer_size);
        el_base64_encode(jpeg_img.data, jpeg_img.size, buffer);
        return concat_strings("\"image\": \"", buffer, "\"");
    }

    return "\"image\": \"\"";
}

std::string algorithm_info_2_json_str(const el_algorithm_info_t* info) {
    return concat_strings("{\"type\": ",
                          std::to_string(info->type),
                          ", \"categroy\": ",
                          std::to_string(info->categroy),
                          ", \"input_from\": ",
                          std::to_string(info->input_from),
                          "}");
}

template <typename AlgorithmConfigType> std::string algorithm_config_2_json_str(const AlgorithmConfigType& config) {
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

template <typename AlgorithmType> std::string algorithm_config_2_json_str(std::shared_ptr<AlgorithmType> algorithm) {
    return algorithm_config_2_json_str(algorithm->get_algorithm_config());
}

template <typename AlgorithmType> std::string algorithm_results_2_json_str(std::shared_ptr<AlgorithmType> algorithm) {
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

std::vector<std::string> tokenize_function_2_argv(const std::string& input) {
    std::vector<std::string> argv;

    std::size_t index = 0;
    std::size_t size  = input.size();
    char        c     = {};

    while (index < size) {
        c = input.at(index);
        if (std::isalnum(c) || c == '_') [[likely]] {
            size_t prev = index;
            while (++index < size) {
                c = input.at(index);
                if (!(std::isalnum(c) || c == '_')) [[unlikely]]
                    break;
            }
            argv.push_back(input.substr(prev, index - prev));
        } else
            ++index;
    }
    argv.shrink_to_fit();

    return argv;
}

}  // namespace sscma::utility
