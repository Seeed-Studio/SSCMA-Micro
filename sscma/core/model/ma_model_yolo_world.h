#ifndef _MA_MODEL_YOLO_WORLD_H
#define _MA_MODEL_YOLO_WORLD_H

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class YoloWorld : public Detector {
   private:
    static constexpr size_t outputs_         = 6;
    static constexpr size_t anchor_variants_ = 3;

    ma_tensor_t output_tensors_[outputs_];

    struct anchor_stride_t {
        size_t stride;
        size_t split;
        size_t size;
        size_t start;
    };

    template <typename T> struct pt_t {
        T x;
        T y;
    };

    std::vector<anchor_stride_t>          anchor_strides_;
    std::vector<std::vector<pt_t<float>>> anchor_matrix_;

    size_t output_scores_ids_[anchor_variants_];
    size_t output_bboxes_ids_[anchor_variants_];

    ma_shape_t       output_shapes_[outputs_];
    ma_quant_param_t output_quant_params_[outputs_];

    static auto dequantValue_ = [](size_t idx, const int8_t* output_array, int32_t zero_point, float scale) {
        return static_cast<float>(output_array[idx] - zero_point) * scale;
    };

    static auto generateAnchorMatrix_ =
      [](const std::vector<anchor_stride_t>& anchor_strides, float shift_right = 1.f, float shift_down = 1.f) {
          const auto                            anchor_matrix_size = anchor_strides.size();
          std::vector<std::vector<pt_t<float>>> anchor_matrix(anchor_matrix_size);
          const float                           shift_right_init = shift_right * 0.5f;
          const float                           shift_down_init  = shift_down * 0.5f;

          for (size_t i = 0; i < anchor_matrix_size; ++i) {
              const auto& anchor_stride   = anchor_strides[i];
              const auto  split           = anchor_stride.split;
              const auto  size            = anchor_stride.size;
              auto&       anchor_matrix_i = anchor_matrix[i];

              anchor_matrix[i].resize(size);

              for (size_t j = 0; j < size; ++j) {
                  float x            = static_cast<float>(j % split) * shift_right + shift_right_init;
                  float y            = static_cast<float>(j / split) * shift_down + shift_down_init;
                  anchor_matrix_i[j] = {x, y};
              }
          }

          return anchor_matrix;
      };

   protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessI8();
    ma_err_t postprocessF32();

   public:
    YoloWorld(Engine* engine);
    ~YoloWorld();
    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif  // _MA_MODEL_YOLO_H
