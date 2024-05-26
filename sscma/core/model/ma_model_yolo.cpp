#include <algorithm>
#include <forward_list>
#include <vector>

#include "core/cv/ma_cv.h"
#include "core/utils/ma_nms.h"


#include "core/model/ma_model_yolo.h"

namespace ma::model {

const static char* TAG = "ma::model::yolo";

Yolo::Yolo(Engine* p_engine_) : Model(p_engine_, "yolo") {
    MA_ASSERT(p_engine_ != nullptr);

    input_  = p_engine_->get_input(0);
    output_ = p_engine_->get_output(0);

    input_img_.width  = input_.shape.dims[1];
    input_img_.height = input_.shape.dims[2];
    input_img_.size   = input_.shape.dims[1] * input_.shape.dims[2] * input_.shape.dims[3];
    input_img_.data   = input_.data.u8;
    if (input_.shape.dims[3] == 3) {
        input_img_.format = MA_PIXEL_FORMAT_RGB888;
    } else if (input_.shape.dims[3] == 1) {
        input_img_.format = MA_PIXEL_FORMAT_GRAYSCALE;
    }
}

Yolo::~Yolo() {}

bool Yolo::is_valid() {
    const auto& input_shape{p_engine_->get_input_shape(0)};
    if (input_shape.size != 4 ||                      // B, W, H, C
        input_shape.dims[0] != 1 ||                   // B = 1
        input_shape.dims[1] ^ input_shape.dims[2] ||  // W = H
        input_shape.dims[1] < 32 ||                   // W, H >= 32
        input_shape.dims[1] % 32 ||                   // W or H is multiply of 32
        (input_shape.dims[3] != 3 &&                  // C = RGB or Gray
         input_shape.dims[3] != 1))
        return false;

    auto        ibox_len{[&]() {
        auto r{static_cast<uint16_t>(input_shape.dims[1])};
        auto s{r >> 5};  // r / 32
        auto m{r >> 4};  // r / 16
        auto l{r >> 3};  // r / 8
        return (s * s + m * m + l * l) * input_shape.dims[3];
    }()};

    const auto& output_shape{p_engine_->get_output_shape(0)};
    if (output_shape.size != 3 ||            // B, IB, BC...
        output_shape.dims[0] != 1 ||         // B = 1
        output_shape.dims[1] != ibox_len ||  // IB is based on input shape
        output_shape.dims[2] < 6 ||          // 6 <= BC - 5[XYWHC] <= 80 (could be larger than 80)
        output_shape.dims[2] > 85)
        return false;

    return true;
}


ma_err_t Yolo::preprocess(const void* input) {

    MA_ASSERT(input != nullptr);

    ma_err_t        ret     = MA_OK;
    const ma_img_t* src_img = static_cast<const ma_img_t*>(input);

    ret = ma::cv::convert(src_img, &input_img_);
    if (ret != MA_OK) {
        return ret;
    }
    if (input_.type == MA_TENSOR_TYPE_S8) {
        for (int i = 0; i < input_.size; i++) {
            printf("%d %c", input_.data.s8[i], i % 63 == 0 ? '\n' : ' ');
            input_.data.s8[i] = input_.data.s8[i] - 128;
        }
    }


    return ret;
}


ma_err_t Yolo::postprocess() {
    results_.clear();

    std::forward_list<ma_bbox_t> result;

    auto                         width{input_.shape.dims[1]};
    auto                         height{input_.shape.dims[2]};

    auto                         scale{output_.quant_param.scale};
    auto                         zero_point{output_.quant_param.zero_point};

    bool                         rescale{scale < 0.1f ? true : false};

    auto                         num_record{output_.shape.dims[1]};
    auto                         num_element{output_.shape.dims[2]};
    auto                         num_class{num_element - 5};

    MA_LOGI(
        TAG, "num_record: %d, num_element: %d, num_class: %d", num_record, num_element, num_class);
    MA_LOGI(TAG, "scale: %f, zero_point: %d", scale, zero_point);

    // parse output
    if (output_.type == MA_TENSOR_TYPE_S8) {
        auto data = output_.data.s8;
        for (decltype(num_record) i{0}; i < num_record; ++i) {
            auto idx{i * num_element};
            auto score{static_cast<decltype(scale)>(data[idx + INDEX_S] - zero_point) * scale};
            score = rescale ? score : score / 100.0f;
            MA_LOGW(TAG, "score: %d=%f", data[idx + INDEX_S], score);
            if (score > 0.5f) {
                ma_bbox_t box{
                    .x      = 0,
                    .y      = 0,
                    .w      = 0,
                    .h      = 0,
                    .score  = score,
                    .target = 0,
                };

                // get box target
                int8_t max{-128};
                for (decltype(num_class) t{0}; t < num_class; ++t) {
                    if (max < data[idx + INDEX_T + t]) {
                        max        = data[idx + INDEX_T + t];
                        box.target = t;
                    }
                }

                // get box position, int8_t - int32_t (narrowing)
                auto x{((data[idx + INDEX_X] - zero_point) * scale)};
                auto y{((data[idx + INDEX_Y] - zero_point) * scale)};
                auto w{((data[idx + INDEX_W] - zero_point) * scale)};
                auto h{((data[idx + INDEX_H] - zero_point) * scale)};

                if (!rescale) {
                    x = x / width;
                    y = y / height;
                    w = w / width;
                    h = h / height;
                }

                box.x = MA_CLIP(x, 0, 1.0f);
                box.y = MA_CLIP(y, 0, 1.0f);
                box.w = MA_CLIP(w, 0, 1.0f);
                box.h = MA_CLIP(h, 0, 1.0f);

                result.emplace_front(std::move(box));
            }
        }
    }

    ma::utils::nms(result, 0.5f, 0.5f);

    result.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x > b.x; });  // left to right

    std::copy(result.begin(), result.end(), std::back_inserter(results_));

    return MA_OK;
}

ma_err_t Yolo::run(const void* input) {
    MA_ASSERT(input != nullptr);
    return underlying_run(input);
}


std::vector<ma_bbox_t>& Yolo::get_results() {
    return results_;
}


}  // namespace ma::model