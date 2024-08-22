#include <gtest/gtest.h>

#include <sscma.h>

#include "yolo_meter.h"

namespace ma::engine {


#define TAG               "test::engine"
#define TENSOR_ARENA_SIZE (1024 * 1024)

uint8_t tensor_arena[TENSOR_ARENA_SIZE];


TEST(Engine, Init) {
    auto* engine1 = new EngineTFLite();
    auto* engine2 = new EngineTFLite();
    auto* engine3 = new EngineTFLite();

    EXPECT_EQ(MA_OK, engine1->init());
    EXPECT_EQ(MA_OK, engine2->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine3->init(tensor_arena, TENSOR_ARENA_SIZE));

    delete engine1;
    delete engine2;
    delete engine3;
}

TEST(Engine, load) {
    auto* engine = new EngineTFLite();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load(g_yolo_meter_model_data, g_yolo_meter_model_data_len));
    delete engine;
}

TEST(Engine, Run) {
    auto* engine = new EngineTFLite();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load(g_yolo_meter_model_data, g_yolo_meter_model_data_len));
    auto input_shape = engine->getInputShape(0);
    EXPECT_EQ(4, input_shape.size);
    EXPECT_EQ(1, input_shape.dims[0]);
    EXPECT_EQ(192, input_shape.dims[1]);
    EXPECT_EQ(192, input_shape.dims[2]);
    EXPECT_EQ(3, input_shape.dims[3]);
    EXPECT_EQ(MA_OK, engine->run());
    auto output_shape = engine->getOutputShape(0);
    EXPECT_EQ(3, output_shape.size);
    EXPECT_EQ(1, output_shape.dims[0]);
    EXPECT_EQ(2268, output_shape.dims[1]);
    EXPECT_EQ(15, output_shape.dims[2]);
    delete engine;
}

#ifdef MA_USE_FILESYSTEM
TEST(Engine, loadFile) {
    auto* engine = new EngineTFLite();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load("yolo_meter.tflite"));
    delete engine;
}

TEST(Engine, RunFile) {
    auto* engine = new EngineTFLite();
    EXPECT_EQ(MA_OK, engine->init(TENSOR_ARENA_SIZE));
    EXPECT_EQ(MA_OK, engine->load("yolo_meter.tflite"));
    auto input_shape = engine->getInputShape(0);
    EXPECT_EQ(4, input_shape.size);
    EXPECT_EQ(1, input_shape.dims[0]);
    EXPECT_EQ(192, input_shape.dims[1]);
    EXPECT_EQ(192, input_shape.dims[2]);
    EXPECT_EQ(3, input_shape.dims[3]);
    EXPECT_EQ(MA_OK, engine->run());
    auto output_shape = engine->getOutputShape(0);
    EXPECT_EQ(3, output_shape.size);
    EXPECT_EQ(1, output_shape.dims[0]);
    EXPECT_EQ(2268, output_shape.dims[1]);
    EXPECT_EQ(15, output_shape.dims[2]);
    delete engine;
}
#endif


}  // namespace ma::engine
