#include <gtest/gtest.h>


#include <sscma.h>

#include "img_meter.h"
#include "yolo_meter.h"

namespace ma::engine {

#define TAG               "test::model"
#define TENSOR_ARENA_SIZE (1024 * 1024)


TEST(Model, YOLO) {
    auto* engine = new Engine();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load_model("yolo_meter.tflite"));
    auto*    model = new ma::model::Yolo(engine);
    ma_img_t img;
    img.data   = (uint8_t*)gImage_meter;
    img.size   = sizeof(gImage_meter);
    img.width  = 240;
    img.height = 240;
    img.format = MA_PIXEL_FORMAT_RGB888;
    img.rotate = MA_PIXEL_ROTATE_0;

    EXPECT_EQ(MA_OK, model->run(static_cast<const void*>(&img)));
    auto perf = model->get_perf();
    MA_LOGI(
        TAG, "pre: %ldms, infer: %ldms, post: %ldms", perf.preprocess, perf.run, perf.postprocess);
    auto _results = model->get_results();
    int  value    = 0;
    for (int i = 0; i < _results.size(); i++) {
        auto result = _results[i];
        MA_LOGI(TAG,
                "class: %d, score: %f, x: %f, y: %f, w: %f, h: %f",
                result.target,
                result.score,
                result.x,
                result.y,
                result.w,
                result.h);
        value = value * 10 + result.target;
    }

    EXPECT_EQ(179295, value);

    delete engine;
}

}  // namespace ma::engine
