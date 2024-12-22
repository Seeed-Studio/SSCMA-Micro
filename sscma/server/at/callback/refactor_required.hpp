#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <forward_list>

#include "core/ma_core.h"
#include "core/ma_debug.h"
#include "core/utils/ma_base64.h"
#include "porting/ma_porting.h"
#include "resource.hpp"
#include "server/at/codec/ma_codec.h"

namespace ma {

#if MA_TRIGGER_USE_REAL_GPIO
extern bool __ma_is_trigger_gpio(int pin);
extern void __ma_init_gpio_level(int pin, int level);
extern void __ma_set_gpio_level(int pin, int level);
#else
static bool __ma_is_trigger_gpio(int pin) {
    (void)pin;
    MA_LOGD(MA_TAG, "GPIO pin: %d", pin);
    return true;
}
static void __ma_init_gpio_level(int pin, int level) {
    (void)pin;
    (void)level;
    MA_LOGD(MA_TAG, "GPIO pin: %d, level: %d", pin, level);
}
static void __ma_set_gpio_level(int pin, int level) {
    (void)pin;
    (void)level;
    MA_LOGD(MA_TAG, "GPIO pin: %d, level: %d", pin, level);
}
#endif

using namespace ma::model;

ma_err_t setAlgorithmInput(Model* algorithm, ma_img_t& img) {
    if (algorithm == nullptr) {
        return MA_EINVAL;
    }

    switch (algorithm->getType()) {
        case MA_MODEL_TYPE_PFLD:
            return static_cast<PointDetector*>(algorithm)->run(&img);

        case MA_MODEL_TYPE_IMCLS:
            return static_cast<Classifier*>(algorithm)->run(&img);

        case MA_MODEL_TYPE_FOMO:
        case MA_MODEL_TYPE_YOLOV5:
        case MA_MODEL_TYPE_YOLOV8:
        case MA_MODEL_TYPE_NVIDIA_DET:
        case MA_MODEL_TYPE_YOLO_WORLD:
        case MA_MODEL_TYPE_RTMDET:
            return static_cast<Detector*>(algorithm)->run(&img);

        case MA_MODEL_TYPE_YOLOV8_POSE:
        case MA_MODEL_TYPE_YOLO11_POSE:
            return static_cast<PoseDetector*>(algorithm)->run(&img);

        default:
            return MA_ENOTSUP;
    }
}

ma_err_t serializeAlgorithmOutput(Model* algorithm, Encoder* encoder, int width, int height) {

    if (algorithm == nullptr || encoder == nullptr) {
        return MA_EINVAL;
    }

    auto ret = MA_OK;

    switch (algorithm->getType()) {
        case MA_MODEL_TYPE_PFLD: {
            auto results = static_cast<PointDetector*>(algorithm)->getResults();
            for (auto& result : results) {
                result.x = static_cast<int>(std::round(result.x * width));
                result.y = static_cast<int>(std::round(result.y * height));
                result.score = static_cast<int>(std::round(result.score * 100));
            }
            ret = encoder->write(results);

            break;
        }

        case MA_MODEL_TYPE_IMCLS: {

            auto results = static_cast<Classifier*>(algorithm)->getResults();
            for (auto& result : results) {
                result.score = static_cast<int>(std::round(result.score * 100));
            }
            ret = encoder->write(results);

            break;
        }

        case MA_MODEL_TYPE_FOMO:
        case MA_MODEL_TYPE_YOLOV5:
        case MA_MODEL_TYPE_YOLOV8:
        case MA_MODEL_TYPE_YOLO11:
        case MA_MODEL_TYPE_NVIDIA_DET:
        case MA_MODEL_TYPE_YOLO_WORLD:
        case MA_MODEL_TYPE_RTMDET: {

            auto results = static_cast<Detector*>(algorithm)->getResults();
            MA_LOGD(MA_TAG, "Results size: %d", std::distance(results.begin(), results.end()));
            for (auto& result : results) {
                result.x = static_cast<int>(std::round(result.x * width));
                result.y = static_cast<int>(std::round(result.y * height));
                result.w = static_cast<int>(std::round(result.w * width));
                result.h = static_cast<int>(std::round(result.h * height));
                result.score = static_cast<int>(std::round(result.score * 100));
            }
            ret = encoder->write(results);

            break;
        }

        case MA_MODEL_TYPE_YOLOV8_POSE:
        case MA_MODEL_TYPE_YOLO11_POSE: {

            auto results = static_cast<PoseDetector*>(algorithm)->getResults();
            for (auto& result : results) {
                auto& box = result.box;
                box.x = static_cast<int>(std::round(box.x * width));
                box.y = static_cast<int>(std::round(box.y * height));
                box.w = static_cast<int>(std::round(box.w * width));
                box.h = static_cast<int>(std::round(box.h * height));
                box.score = static_cast<int>(std::round(box.score * 100));
                for (auto& pt : result.pts) {
                    pt.x = static_cast<int>(std::round(pt.x * width));
                    pt.y = static_cast<int>(std::round(pt.y * height));
                }
            }
            ret = encoder->write(results);

            break;
        }

        default:
            ret = MA_ENOTSUP;
    }

    if (ret != MA_OK) {
        MA_LOGD(MA_TAG, "Failed to serialize algorithm output: %d", ret);
    }

    return ret;
}

struct TriggerRule {
    static std::shared_ptr<TriggerRule> create(const std::string& rule_str) {
        auto rule = std::make_shared<TriggerRule>();

        std::vector<std::string> tokens;
        for (size_t i = 0, j = 0; i < rule_str.size(); i = j + 1) {
            j = rule_str.find(',', i);
            if (j == std::string::npos) {
                j = rule_str.size();
            }
            tokens.push_back(rule_str.substr(i, j - i));
        }
        if (tokens.size() < 6) {
            return nullptr;
        }

        rule->class_id = std::atoi(tokens[0].c_str());
        if (rule->class_id < 0) {
            return nullptr;
        }

        switch (std::atoi(tokens[1].c_str())) {
            case 0:
                rule->comp = std::greater<float>();
                break;
            case 1:
                rule->comp = std::less<float>();
                break;
            case 2:
                rule->comp = std::greater_equal<float>();
                break;
            case 3:
                rule->comp = std::less_equal<float>();
                break;
            case 4:
                rule->comp = std::equal_to<float>();
                break;
            case 5:
                rule->comp = std::not_equal_to<float>();
                break;
            default:
                return nullptr;
        }

        rule->threshold = std::atoi(tokens[2].c_str()) / 100.0;
        if (rule->threshold < 0 || rule->threshold > 1) {
            return nullptr;
        }

        rule->gpio_pin = std::atoi(tokens[3].c_str());
        if (!__ma_is_trigger_gpio(rule->gpio_pin)) {
            return nullptr;
        }

        rule->default_level = std::atoi(tokens[4].c_str());
        if (rule->default_level != 0 && rule->default_level != 1) {
            return nullptr;
        }
        rule->trigger_level = std::atoi(tokens[5].c_str());
        if (rule->trigger_level != 0 && rule->trigger_level != 1) {
            return nullptr;
        }

        __ma_init_gpio_level(rule->gpio_pin, rule->default_level);

        return rule;
    }

    ~TriggerRule() = default;

    void operator()(Model* algorithm) {
        if (algorithm == nullptr) {
            return;
        }

        bool fit = false;

        switch (algorithm->getType()) {
            case MA_MODEL_TYPE_PFLD: {
                const auto& results = static_cast<PointDetector*>(algorithm)->getResults();
                for (const auto& result : results) {
                    if (result.target == class_id && comp(result.score, threshold)) {
                        fit = true;
                        break;
                    }
                }
                break;
            }

            case MA_MODEL_TYPE_IMCLS: {
                auto results = static_cast<Classifier*>(algorithm)->getResults();
                for (auto& result : results) {
                    if (result.target == class_id && comp(result.score, threshold)) {
                        fit = true;
                        break;
                    }
                }
                break;
            }

            case MA_MODEL_TYPE_FOMO:
            case MA_MODEL_TYPE_YOLOV5:
            case MA_MODEL_TYPE_YOLOV8:
            case MA_MODEL_TYPE_YOLO11:
            case MA_MODEL_TYPE_NVIDIA_DET:
            case MA_MODEL_TYPE_YOLO_WORLD: 
            case MA_MODEL_TYPE_RTMDET: {
                auto results = static_cast<Detector*>(algorithm)->getResults();
                for (auto& result : results) {
                    if (result.target == class_id && comp(result.score, threshold)) {
                        fit = true;
                        break;
                    }
                }

                break;
            }

            case MA_MODEL_TYPE_YOLOV8_POSE:
            case MA_MODEL_TYPE_YOLO11_POSE: {
                auto results = static_cast<PoseDetector*>(algorithm)->getResults();
                for (auto& result : results) {
                    if (result.box.target == class_id && comp(result.box.score, threshold)) {
                        fit = true;
                        break;
                    }
                }

                break;
            }

            default:
                break;
        }

        if (fit) {
            __ma_set_gpio_level(gpio_pin, trigger_level);
        } else {
            __ma_set_gpio_level(gpio_pin, default_level);
        }
    }

    TriggerRule() = default;

private:
    int class_id;
    std::function<bool(float, float)> comp;
    float threshold;
    int gpio_pin;
    int default_level;
    int trigger_level;
};

std::forward_list<std::shared_ptr<TriggerRule>> trigger_rules;
ma::Mutex trigger_rules_mutex;

}  // namespace ma
