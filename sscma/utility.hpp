#pragma once

#include <array>
#include <cstdint>
#include <forward_list>
#include <iomanip>
#include <sstream>
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

namespace sscma::utility {

using namespace edgelab;
using namespace edgelab::utility;

std::string quoted_stringify(const std::string& str) {
    std::string ss(1, '"');
    for (char c : str) {
        if (c == '"') [[unlikely]]
            ss += "\\\"";
        else if (c == '\n') [[unlikely]]
            ss += "\\n";
        else if (std::isprint(c)) [[likely]]
            ss += c;
    }
    ss += '"';
    return ss;
}

inline uint32_t color_literal(uint8_t i) {
    static const uint16_t color[] = {
      0x0000,
      0x03E0,
      0x001F,
      0x7FE0,
      0xFFFF,
    };
    return color[i % 5];
}

void draw_results_on_image(const std::forward_list<el_point_t>& results, el_img_t* img) {
    uint8_t i = 0;
    for (const auto& point : results) edgelab::el_draw_point(img, point.x, point.y, color_literal(++i));
}

void draw_results_on_image(const std::forward_list<el_box_t>& results, el_img_t* img) {
    uint8_t i = 0;
    for (const auto& box : results) {
        int16_t y = box.y - (box.h >> 1);
        int16_t x = box.x - (box.w >> 1);
        edgelab::el_draw_rect(img, x, y, box.w, box.h, color_literal(++i), 4);
    }
}

std::string model_info_2_json_str(el_model_info_t model_info) {
    auto os{std::ostringstream(std::ios_base::ate)};

    os << "{\"id\": " << static_cast<unsigned>(model_info.id)
       << ", \"type\": " << static_cast<unsigned>(model_info.type) << ", \"address\": \"0x" << std::hex
       << static_cast<unsigned>(model_info.addr_flash) << "\", \"size\": \"0x" << static_cast<unsigned>(model_info.size)
       << "\"}";

    return std::string(os.str());
}

std::string sensor_info_2_json_str(el_sensor_info_t sensor_info) {
    auto os{std::ostringstream(std::ios_base::ate)};

    os << "{\"id\": " << static_cast<unsigned>(sensor_info.id)
       << ", \"type\": " << static_cast<unsigned>(sensor_info.type)
       << ", \"state\": " << static_cast<unsigned>(sensor_info.state) << "}";

    return std::string(os.str());
}

template <typename T> constexpr std::string results_2_json_str(const std::forward_list<T>& results) {
    auto os{std::ostringstream(std::ios_base::ate)};

    DELIM_RESET;
    if constexpr (std::is_same<T, el_box_t>::value) {
        os << "\"boxes\": [";
        for (const auto& box : results) {
            DELIM_PRINT(os);
            os << "[" << static_cast<unsigned>(box.x) << ", " << static_cast<unsigned>(box.y) << ", "
               << static_cast<unsigned>(box.w) << ", " << static_cast<unsigned>(box.h) << ", "
               << static_cast<unsigned>(box.score) << ", " << static_cast<unsigned>(box.target) << "]";
        }
    } else if constexpr (std::is_same<T, el_point_t>::value) {
        os << "\"points\": [";
        for (const auto& point : results) {
            DELIM_PRINT(os);
            os << "[" << static_cast<unsigned>(point.x) << ", " << static_cast<unsigned>(point.y) << ", "
               << static_cast<unsigned>(point.score) << ", " << static_cast<unsigned>(point.target) << "]";
        }
    } else if constexpr (std::is_same<T, el_class_t>::value) {
        os << "\"classes\": [";
        for (const auto& cls : results) {
            DELIM_PRINT(os);
            os << "[" << static_cast<unsigned>(cls.score) << ", " << static_cast<unsigned>(cls.target) << "]";
        }
    }
    os << "]";

    return std::string(os.str());
}

// TODO: avoid repeatly allocate/release memory in for loop
std::string img_2_jpeg_json_str(const el_img_t* img) {
    auto os = std::ostringstream(std::ios_base::ate);

    if (!img || !img->data) [[unlikely]]
        return "\"image\": \"\"";

    os << "\"image\": \"";
    size_t size     = img->width * img->height * 3;
    auto   jpeg_img = el_img_t{.data   = new uint8_t[size]{},
                               .size   = size,
                               .width  = img->width,
                               .height = img->height,
                               .format = EL_PIXEL_FORMAT_JPEG,
                               .rotate = img->rotate};

    el_err_code_t ret = el_img_convert(img, &jpeg_img);
    if (ret == EL_OK) [[likely]] {
        auto* buffer = new char[((jpeg_img.size + 2) / 3) * 4 + 1]{};
        el_base64_encode(jpeg_img.data, jpeg_img.size, buffer);
        os << buffer;
        delete[] buffer;
    }
    delete[] jpeg_img.data;
    os << "\"";

    return std::string(os.str());
}

std::string algorithm_info_2_json_str(const el_algorithm_info_t* info) {
    auto os{std::ostringstream(std::ios_base::ate)};

    os << "{\"type\": " << static_cast<unsigned>(info->type)
       << ", \"categroy\": " << static_cast<unsigned>(info->categroy)
       << ", \"input_from\": " << static_cast<unsigned>(info->input_from) << "}";

    return std::string(os.str());
}

template <typename InfoConfType> std::string algorithm_info_and_conf_2_json_str(const InfoConfType& info_and_conf) {
    auto os{std::ostringstream(std::ios_base::ate)};

    os << "{\"type\": " << static_cast<unsigned>(info_and_conf.info.type)
       << ", \"categroy\": " << static_cast<unsigned>(info_and_conf.info.categroy)
       << ", \"input_from\": " << static_cast<unsigned>(info_and_conf.info.input_from) << ", \"config\": {";
    if constexpr (std::is_same<InfoConfType, el_algorithm_fomo_config_t>::value ||
                  std::is_same<InfoConfType, el_algorithm_imcls_config_t>::value)
        os << "\"tscore\": " << static_cast<unsigned>(info_and_conf.score_threshold);
    else if constexpr (std::is_same<InfoConfType, el_algorithm_yolo_config_t>::value)
        os << "\"tscore\": " << static_cast<unsigned>(info_and_conf.score_threshold)
           << ", \"tiou\": " << static_cast<unsigned>(info_and_conf.iou_threshold);
    os << "}}";

    return std::string(os.str());
}

template <typename AlgorithmType>
std::string img_invoke_results_2_json_str(
  const AlgorithmType* algorithm, const el_img_t* img, const std::string& cmd, bool result_only, el_err_code_t ret) {
    auto os{std::ostringstream(std::ios_base::ate)};

    os << REPLY_EVT_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"perf\": [" << static_cast<unsigned>(algorithm->get_preprocess_time()) << ", "
       << static_cast<unsigned>(algorithm->get_run_time()) << ", "
       << static_cast<unsigned>(algorithm->get_postprocess_time()) << "], "
       << results_2_json_str(algorithm->get_results());
    if (!result_only) os << ", " << img_2_jpeg_json_str(img);
    os << "}}\n";

    return std::string(os.str());
}

std::vector<std::string> tokenize_function_2_argv(const std::string& input) {
    std::vector<std::string> argv;

    size_t index = 0;
    size_t size  = input.size();
    char   c     = {};

    while (index < size) {
        c = input.at(index);
        if (std::isalnum(c) || c == '_') {
            size_t prev = index;
            while (++index < size) {
                c = input.at(index);
                if (!std::isalnum(c) && c != '_') [[unlikely]]
                    break;
            }
            argv.push_back(input.substr(prev, index - prev));
        } else
            ++index;
    }
    argv.shrink_to_fit();

    return argv;
}

std::string action_str_2_json_str(const char* str) {
    std::ostringstream         os;
    std::string                buf;
    std::stringstream          ss(str);
    std::array<std::string, 3> argv_3{};

    for (size_t i = 0; i < argv_3.size() && std::getline(ss, buf, '\t'); ++i) argv_3[i] = buf;

    os << "\"cond\": " << quoted_stringify(argv_3[0]) << ", \"true\": " << quoted_stringify(argv_3[1])
       << ", \"false\": " << quoted_stringify(argv_3[2]) << ", \"exception\": " << quoted_stringify(argv_3[3]);

    return std::string(os.str());
}

std::string action_str_2_cmd_str(const char* str) {
    std::ostringstream         os;
    std::string                buf;
    std::stringstream          ss(str);
    std::array<std::string, 3> argv_3{};

    for (size_t i = 0; i < argv_3.size() && std::getline(ss, buf, '\t'); ++i) argv_3[i] = buf;

    os << "AT+ACTION=" << quoted_stringify(argv_3[0]) << "," << quoted_stringify(argv_3[1]) << ","
       << quoted_stringify(argv_3[2]);

    return std::string(os.str());
}

}  // namespace sscma::utility
