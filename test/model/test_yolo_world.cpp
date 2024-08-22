#include <gtest/gtest.h>

#include "core/model/ma_model_yolo_world.h"

#include <memory>
#include <new>
#include <random>
#include <string>

namespace ma {

using namespace ma::engine;
using namespace ma::model;

class FakeEngine : public Engine {
public:
    FakeEngine() = default;

    ~FakeEngine() override = default;

    ma_err_t load(const void* model_data, size_t model_size) override {
        return MA_OK;
    }

#if MA_USE_FILESYSTEM
    ma_err_t load(const char* model_path) override {
        return MA_OK;
    }

    ma_err_t load(const std::string& model_path) override {
        return MA_OK;
    }
#endif

    ma_err_t init() override {
        return MA_OK;
    }

    ma_err_t init(size_t size) override {
        return MA_OK;
    }

    ma_err_t init(void* pool, size_t size) override {
        return MA_OK;
    }

    ma_err_t run() override {
        return MA_OK;
    }

    int32_t getInputSize() override {
        return 1;
    }

    int32_t getOutputSize() override {
        return 6;
    }

    ma_tensor_t getInput(int32_t index) override {
        return ma_tensor_t();
    }


    ma_tensor_t getOutput(int32_t index) override {
        return ma_tensor_t();
    }

    ma_shape_t getInputShape(int32_t index) override {
        switch (index) {
            case 0:
                return {4, {1, 320, 320, 3}};

            default:
                return {0, {0}};
        }
    }

    ma_shape_t getOutputShape(int32_t index) override {
        switch (index) {
            case 0:
                return {3, {1, 1600, 1}};

            case 1:
                return {3, {1, 100, 1}};

            case 2:
                return {3, {1, 100, 64}};

            case 3:
                return {3, {1, 400, 64}};

            case 4:
                return {3, {1, 400, 1}};

            case 5:
                return {3, {1, 1600, 64}};

            default:
                return {0, {0}};
        }
    }

    ma_quant_param_t getInputQuantParam(int32_t index) override {
        return {0.f, 0};
    }

    ma_quant_param_t getOutputQuantParam(int32_t index) override {
        return {0.f, 0};
    }
};

class AnotherFakeEngine : public FakeEngine {
public:
    AnotherFakeEngine() = default;

    ~AnotherFakeEngine() override = default;

    ma_shape_t getOutputShape(int32_t index) override {
        switch (index) {
            case 3:
                return {3, {1, 1600, 1}};

            case 1:
                return {3, {1, 100, 1}};

            case 0:
                return {3, {1, 100, 64}};

            case 5:
                return {3, {1, 400, 64}};

            case 4:
                return {3, {1, 400, 1}};

            case 2:
                return {3, {1, 1600, 64}};

            default:
                return {0, {0}};
        }
    }
};

class AnotherAnotherFakeEngine : public FakeEngine {
public:
    AnotherAnotherFakeEngine() = default;

    ~AnotherAnotherFakeEngine() override = default;

    ma_shape_t getOutputShape(int32_t index) override {
        switch (index) {
            case 3:
                return {3, {1, 1600, 4}};

            case 1:
                return {3, {1, 100, 4}};

            case 0:
                return {3, {1, 100, 64}};

            case 5:
                return {3, {1, 400, 64}};

            case 4:
                return {3, {1, 400, 4}};

            case 2:
                return {3, {1, 1600, 64}};

            default:
                return {0, {0}};
        }
    }
};

class BadEngine : public FakeEngine {
public:
    BadEngine() = default;

    ~BadEngine() override = default;

    int32_t getOutputSize() override {
        static std::vector<int32_t> sizes = {0, 1, 2, 3, 4, 5, 7};
        static size_t index               = 0;

        return sizes[index++ % sizes.size()];
    }
};

class AnotherBadEngine : public FakeEngine {
public:
    AnotherBadEngine() = default;

    ~AnotherBadEngine() override = default;

    ma_shape_t getOutputShape(int32_t index) override {
        switch (index) {
            case 3:
                return {3, {1, 1, 1600}};

            case 1:
                return {3, {1, 100, 1}};

            case 0:
                return {3, {1, 100, 64}};

            case 5:
                return {3, {1, 400, 64}};

            case 4:
                return {3, {1, 400, 1}};

            case 2:
                return {3, {1, 1600, 64}};

            default:
                return {0, {0}};
        }
    }
};


TEST(MODEL, YOLOWorldIsValid) {
    auto fake_engine = std::unique_ptr<Engine>(new FakeEngine());

    EXPECT_EQ(YoloWorld::isValid(fake_engine.get()), true);

    auto another_fake_engine = std::unique_ptr<Engine>(new AnotherFakeEngine());

    EXPECT_EQ(YoloWorld::isValid(another_fake_engine.get()), true);

    auto another_another_fake_engine = std::unique_ptr<Engine>(new AnotherAnotherFakeEngine());

    EXPECT_EQ(YoloWorld::isValid(another_another_fake_engine.get()), true);

    for (int i = 0; i < 7; i++) {
        auto bad_engine = std::unique_ptr<Engine>(new BadEngine());

        EXPECT_EQ(YoloWorld::isValid(bad_engine.get()), false);
    }

    auto another_bad_engine = std::unique_ptr<Engine>(new AnotherBadEngine());

    EXPECT_EQ(YoloWorld::isValid(another_bad_engine.get()), false);
}

}  // namespace ma
