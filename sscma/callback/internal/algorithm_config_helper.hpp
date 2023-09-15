#pragma once

#include <cstdint>
#include <forward_list>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>

#include "sscma/static_resourse.hpp"

namespace sscma::callback {

template <typename AlgorithmType> class AlgorithmConfigHelper {
   public:
    using ConfigType = typename AlgorithmType::ConfigType;

    AlgorithmConfigHelper(AlgorithmType* algorithm)
        : _algorithm(algorithm), _config(algorithm->get_algorithm_config()) {
        auto kv = el_make_storage_kv_from_type(_config);
        if (static_resourse->storage->contains(kv.key)) [[likely]]
            *static_resourse->storage >> kv;
        else
            *static_resourse->storage << kv;
        _algorithm->set_algorithm_config(kv.value);

        if constexpr (std::is_same<ConfigType, el_algorithm_fomo_config_t>::value ||
                      std::is_same<ConfigType, el_algorithm_imcls_config_t>::value ||
                      std::is_same<ConfigType, el_algorithm_yolo_config_t>::value) {
            if (static_resourse->instance->register_cmd(
                  "TSCORE", "Set score threshold", "SCORE_THRESHOLD", [this, &kv](std::vector<std::string> argv) {
                      auto    os        = std::ostringstream(std::ios_base::ate);
                      uint8_t value     = std::atoi(argv[1].c_str());  // implicit conversion eliminates negtive values
                      el_err_code_t ret = value <= 100u ? EL_OK : EL_EINVAL;
                      if (ret == EL_OK) [[likely]] {
                          this->_algorithm->set_score_threshold(value);
                          kv.value.score_threshold = value;
                          *static_resourse->storage << kv;
                      }
                      os << REPLY_CMD_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(ret)
                         << ", \"data\": " << static_cast<unsigned>(kv.value.score_threshold) << "}\n";
                      const auto& str{os.str()};
                      static_resourse->transport->send_bytes(str.c_str(), str.size());
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TSCORE");

            if (static_resourse->instance->register_cmd(
                  "TSCORE?", "Get score threshold", "", [this](std::vector<std::string> argv) {
                      auto os = std::ostringstream(std::ios_base::ate);
                      os << REPLY_CMD_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(EL_OK)
                         << ", \"data\": " << static_cast<unsigned>(this->_algorithm->get_score_threshold()) << "}\n";
                      const auto& str{os.str()};
                      static_resourse->transport->send_bytes(str.c_str(), str.size());
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TSCORE?");
        }

        if constexpr (std::is_same<ConfigType, el_algorithm_yolo_config_t>::value) {
            if (static_resourse->instance->register_cmd(
                  "TIOU", "Set IoU threshold", "IOU_THRESHOLD", [this, &kv](std::vector<std::string> argv) {
                      auto    os        = std::ostringstream(std::ios_base::ate);
                      uint8_t value     = std::atoi(argv[1].c_str());  // implicit conversion eliminates negtive values
                      el_err_code_t ret = value <= 100 ? EL_OK : EL_EINVAL;
                      if (ret == EL_OK) [[likely]] {
                          this->_algorithm->set_iou_threshold(value);
                          kv.value.iou_threshold = value;
                          *static_resourse->storage << kv;
                      }
                      os << REPLY_CMD_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(ret)
                         << ", \"data\": " << static_cast<unsigned>(kv.value.iou_threshold) << "}\n";
                      const auto& str{os.str()};
                      static_resourse->transport->send_bytes(str.c_str(), str.size());
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TIOU");

            if (static_resourse->instance->register_cmd(
                  "TIOU?", "Get IoU threshold", "", [this](std::vector<std::string> argv) {
                      auto os = std::ostringstream(std::ios_base::ate);
                      os << REPLY_CMD_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(EL_OK)
                         << ", \"data\": " << static_cast<unsigned>(this->_algorithm->get_iou_threshold()) << "}\n";
                      const auto& str{os.str()};
                      static_resourse->transport->send_bytes(str.c_str(), str.size());
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TIOU?");
        }
    }

    ConfigType dump_config() { return _config; }

    ~AlgorithmConfigHelper() {
        for (const auto& cmd : _config_cmds) static_resourse->instance->unregister_cmd(cmd);
    }

   private:
    std::forward_list<std::string> _config_cmds;

    AlgorithmType* _algorithm;
    ConfigType     _config;
};

}  // namespace sscma::callback
