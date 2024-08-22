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
    MA_LOGD(TAG, "preprocess done");
    preprocess_done_flag = true;
}


void postprocess_done(void* ctx) {
    MA_LOGD(TAG, "postprocess done");
    postprocess_done_flag = true;
}


void inference_done(void* ctx) {
    MA_LOGD(TAG, "inference done");
    inference_done_flag = true;
}


TEST(Model, YOLO) {
    auto* engine = new engine::EngineTFLite();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load("yolo_meter.tflite"));
    Detector* detector = static_cast<Detector*>(new ma::model::YoloV5(engine));
    ma_img_t img;
    img.data   = (uint8_t*)gImage_meter;
    img.size   = sizeof(gImage_meter);
    img.width  = 240;
    img.height = 240;
    img.format = MA_PIXEL_FORMAT_RGB888;
    img.rotate = MA_PIXEL_ROTATE_0;

    detector->setPostprocessDone(postprocess_done);
    detector->setRunDone(inference_done);
    detector->setPreprocessDone(preprocess_done);
    detector->setConfig(MA_MODEL_CFG_OPT_THRESHOLD, 0.6f);
    detector->setConfig(MA_MODEL_CFG_OPT_NMS, 0.45f);
    double threshold_nms   = 0;
    double threshold_score = 0;
    detector->getConfig(MA_MODEL_CFG_OPT_NMS, &threshold_nms);
    detector->getConfig(MA_MODEL_CFG_OPT_THRESHOLD, &threshold_score);
    EXPECT_EQ(threshold_nms, 0.45f);
    EXPECT_EQ(threshold_score, 0.6f);

    EXPECT_EQ(MA_OK, detector->run(&img));

    EXPECT_EQ(true, preprocess_done_flag);
    EXPECT_EQ(true, inference_done_flag);
    EXPECT_EQ(true, postprocess_done_flag);

    auto perf = detector->getPerf();
    MA_LOGD(TAG,
            "pre: %ldms, infer: %ldms, post: %ldms",
            perf.preprocess,
            perf.inference,
            perf.postprocess);
    auto _results = detector->getResults();
    int value     = 0;
    for (auto& result : _results) {
        MA_LOGD(TAG,
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
    auto* engine = new engine::EngineTFLite();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load("mnist.tflite"));
    Classifier* classifier = static_cast<Classifier*>(new ma::model::Classifier(engine));
    ma_img_t img;
    img.data   = (uint8_t*)gImage_mnist_7;
    img.size   = sizeof(gImage_mnist_7);
    img.width  = 64;
    img.height = 64;
    img.format = MA_PIXEL_FORMAT_RGB888;
    img.rotate = MA_PIXEL_ROTATE_0;


    classifier->setConfig(MA_MODEL_CFG_OPT_THRESHOLD, 0.6f);
    double cfg2 = 0;
    classifier->getConfig(MA_MODEL_CFG_OPT_THRESHOLD, &cfg2);

    EXPECT_EQ(cfg2, 0.6f);

    EXPECT_EQ(MA_OK, classifier->run(&img));

    auto perf = classifier->getPerf();
    MA_LOGD(TAG,
            "pre: %ldms, infer: %ldms, post: %ldms",
            perf.preprocess,
            perf.inference,
            perf.postprocess);
    auto _results = classifier->getResults();
    int value     = 0;
    for (auto& result : _results) {
        MA_LOGD(TAG, "class: %d, score: %f", result.target, result.score);
        EXPECT_EQ(7, result.target - 1);
    }

    delete engine;
}


}  // namespace ma::model
