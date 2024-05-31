#include <gtest/gtest.h>


#include <sscma.h>

#include "img_meter.h"
#include "img_mnist.h"
#include "yolo_meter.h"

namespace ma::model {

#define TAG               "test::model"
#define TENSOR_ARENA_SIZE (1024 * 1024)

bool preprocess_done_flag  = false;
bool postprocess_done_flag = false;
bool inference_done_flag   = false;

void preprocess_done(void* ctx) {
    MA_LOGI(TAG, "preprocess done");
    preprocess_done_flag = true;
}


void postprocess_done(void* ctx) {
    MA_LOGI(TAG, "postprocess done");
    postprocess_done_flag = true;
}


void inference_done(void* ctx) {
    MA_LOGI(TAG, "inference done");
    inference_done_flag = true;
}


TEST(Model, YOLO) {
    auto* engine = new Engine();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load_model("yolo_meter.tflite"));
    Detector* detector = static_cast<Detector*>(new ma::model::Yolo(engine));
    ma_img_t  img;
    img.data   = (uint8_t*)gImage_meter;
    img.size   = sizeof(gImage_meter);
    img.width  = 240;
    img.height = 240;
    img.format = MA_PIXEL_FORMAT_RGB888;
    img.rotate = MA_PIXEL_ROTATE_0;

    detector->set_postprocess_done(postprocess_done);
    detector->set_run_done(inference_done);
    detector->set_preprocess_done(preprocess_done);
    detector->set_config(MODEL_CONFIG_OPTION_THRESHOLD, 0.6f);
    detector->set_config(MODEL_CONFIG_OPTION_NMS, 0.45f);
    double threshold_nms   = 0;
    double threshold_score = 0;
    detector->get_config(MODEL_CONFIG_OPTION_NMS, &threshold_nms);
    detector->get_config(MODEL_CONFIG_OPTION_THRESHOLD, &threshold_score);
    EXPECT_EQ(threshold_nms, 0.45f);
    EXPECT_EQ(threshold_score, 0.6f);

    EXPECT_EQ(MA_OK, detector->run(&img));

    EXPECT_EQ(true, preprocess_done_flag);
    EXPECT_EQ(true, inference_done_flag);
    EXPECT_EQ(true, postprocess_done_flag);

    auto perf = detector->get_perf();
    MA_LOGI(
        TAG, "pre: %ldms, infer: %ldms, post: %ldms", perf.preprocess, perf.run, perf.postprocess);
    auto _results = detector->get_results();
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

TEST(Model, Classifier) {
    auto* engine = new Engine();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load_model("mnist.tflite"));
    Classifier* classifier = static_cast<Classifier*>(new ma::model::Classifier(engine));
    ma_img_t    img;
    img.data   = (uint8_t*)gImage_mnist_7;
    img.size   = sizeof(gImage_mnist_7);
    img.width  = 64;
    img.height = 64;
    img.format = MA_PIXEL_FORMAT_RGB888;
    img.rotate = MA_PIXEL_ROTATE_0;


    classifier->set_config(MODEL_CONFIG_OPTION_THRESHOLD, 0.6f);
    double cfg2 = 0;
    classifier->get_config(MODEL_CONFIG_OPTION_THRESHOLD, &cfg2);

    EXPECT_EQ(cfg2, 0.6f);

    EXPECT_EQ(MA_OK, classifier->run(&img));

    auto perf = classifier->get_perf();
    MA_LOGI(
        TAG, "pre: %ldms, infer: %ldms, post: %ldms", perf.preprocess, perf.run, perf.postprocess);
    auto _results = classifier->get_results();
    int  value    = 0;
    for (int i = 0; i < _results.size(); i++) {
        auto result = _results[i];
        MA_LOGI(TAG, "class: %d, score: %f", result.target, result.score);
    }

    EXPECT_EQ(7, _results[0].target - 1);  // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9

    delete engine;
}


}  // namespace ma::model
