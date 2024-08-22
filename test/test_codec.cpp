#include <gtest/gtest.h>

#include <sscma.h>

namespace ma {

static std::string test;
TEST(EncoderTest, EncoderJSONTEST) {
    EncoderJSON encoder;
    EXPECT_EQ(encoder.begin(MA_MSG_TYPE_RESP, MA_OK, "TEST"), MA_OK);
    EXPECT_EQ(encoder.write("i8", static_cast<int8_t>(1)), MA_OK);
    EXPECT_EQ(encoder.write("i16", static_cast<int16_t>(2)), MA_OK);
    EXPECT_EQ(encoder.write("i32", static_cast<int32_t>(3)), MA_OK);
    EXPECT_EQ(encoder.write("i64", static_cast<int64_t>(4)), MA_OK);
    EXPECT_EQ(encoder.write("u8", static_cast<uint8_t>(5)), MA_OK);
    EXPECT_EQ(encoder.write("u16", static_cast<uint16_t>(6)), MA_OK);
    EXPECT_EQ(encoder.write("u32", static_cast<uint32_t>(7)), MA_OK);
    EXPECT_EQ(encoder.write("u64", static_cast<uint64_t>(8)), MA_OK);
    EXPECT_EQ(encoder.write("float", static_cast<float>(9.5f)), MA_OK);
    EXPECT_EQ(encoder.write("double", static_cast<double>(10.5f)), MA_OK);
    EXPECT_EQ(encoder.write("string", std::string("test")), MA_OK);
    EXPECT_EQ(encoder.write("string", std::string("test2")), MA_EEXIST);
    const ma_perf_t perf = {11, 12, 13};
    EXPECT_EQ(encoder.write(perf), MA_OK);
    std::forward_list<ma_class_t> classes;
    classes.emplace_front(ma_class_t{0.86, 0});
    classes.emplace_front(ma_class_t{0.87, 1});
    EXPECT_EQ(encoder.write(classes), MA_OK);
    std::forward_list<ma_point_t> points;
    points.emplace_front(ma_point_t{0.15, 0.32, 0.89, 1});
    points.emplace_front(ma_point_t{0.16, 0.33, 0.90, 1});
    EXPECT_EQ(encoder.write(points), MA_OK);
    std::forward_list<ma_bbox_t> bboxs;
    bboxs.emplace_front(ma_bbox_t{0.1, 0.2, 0.3, 0.4, 0.86, 0});
    bboxs.emplace_front(ma_bbox_t{0.2, 0.3, 0.4, 0.5, 0.87, 1});
    EXPECT_EQ(encoder.write(bboxs), MA_OK);
    EXPECT_EQ(encoder.end(), MA_OK);
    test = encoder.toString();
    MA_LOGD("test", "%s\n", encoder.toString().c_str());
    EXPECT_TRUE(static_cast<bool>(encoder));
}


TEST(DecoderTest, DecoderJSONTEST) {
    DecoderJSON decoder;
    EXPECT_EQ(decoder.begin(test), MA_OK);
    int8_t i8 = 0;
    EXPECT_EQ(decoder.read("i8", i8), MA_OK);
    EXPECT_EQ(i8, 1);
    int16_t i16 = 0;
    EXPECT_EQ(decoder.read("i16", i16), MA_OK);
    EXPECT_EQ(i16, 2);
    int32_t i32 = 0;
    EXPECT_EQ(decoder.read("i32", i32), MA_OK);
    EXPECT_EQ(i32, 3);
    int64_t i64 = 0;
    EXPECT_EQ(decoder.read("i64", i64), MA_OK);
    EXPECT_EQ(i64, 4);
    uint8_t u8 = 0;
    EXPECT_EQ(decoder.read("u8", u8), MA_OK);
    EXPECT_EQ(u8, 5);
    uint16_t u16 = 0;
    EXPECT_EQ(decoder.read("u16", u16), MA_OK);
    EXPECT_EQ(u16, 6);
    uint32_t u32 = 0;
    EXPECT_EQ(decoder.read("u32", u32), MA_OK);
    EXPECT_EQ(u32, 7);
    uint64_t u64 = 0;
    EXPECT_EQ(decoder.read("u64", u64), MA_OK);
    EXPECT_EQ(u64, 8);
    float f = 0;
    EXPECT_EQ(decoder.read("float", f), MA_OK);
    EXPECT_EQ(f, 9.5f);
    double d = 0;
    EXPECT_EQ(decoder.read("double", d), MA_OK);
    EXPECT_EQ(d, 10.5f);
    std::string s;
    EXPECT_EQ(decoder.read("string", s), MA_OK);
    EXPECT_EQ(s, "test");
    ma_perf_t perf = {0, 0, 0};
    EXPECT_EQ(decoder.read(perf), MA_OK);
    EXPECT_EQ(perf.preprocess, 11);
    EXPECT_EQ(perf.inference, 12);
    EXPECT_EQ(perf.postprocess, 13);
    // std::forward_list<ma_class_t> classes;
    // EXPECT_EQ(decoder.read(classes), MA_OK);
    // EXPECT_EQ(classes.size(), 2);
    // EXPECT_NEAR(classes[0].score, 0.86, 1e-4);
    // EXPECT_EQ(classes[0].target, 0);
    // EXPECT_NEAR(classes[1].score, 0.87, 1e-4);
    // EXPECT_EQ(classes[1].target, 1);
    // std::vector<ma_point_t> points;
    // EXPECT_EQ(decoder.read(points), MA_OK);
    // EXPECT_EQ(points.size(), 2);
    // EXPECT_NEAR(points[0].x, 0.15, 1e-4);
    // EXPECT_NEAR(points[0].y, 0.32, 1e-4);
    // EXPECT_NEAR(points[0].score, 0.89, 1e-4);
    // EXPECT_EQ(points[0].target, 1);
    // EXPECT_NEAR(points[1].x, 0.16, 1e-4);
    // EXPECT_NEAR(points[1].y, 0.33, 1e-4);
    // EXPECT_NEAR(points[1].score, 0.90, 1e-4);
    // EXPECT_EQ(points[1].target, 1);
    // std::forward_list<ma_bbox_t> bboxs;
    // EXPECT_EQ(decoder.read(bboxs), MA_OK);
    // EXPECT_EQ(bboxs.size(), 2);
    // EXPECT_NEAR(bboxs[0].x, 0.1, 1e-4);
    // EXPECT_NEAR(bboxs[0].y, 0.2, 1e-4);
    // EXPECT_NEAR(bboxs[0].w, 0.3, 1e-4);
    // EXPECT_NEAR(bboxs[0].h, 0.4, 1e-4);
    // EXPECT_NEAR(bboxs[0].score, 0.86, 1e-4);
    // EXPECT_EQ(bboxs[0].target, 0);
    // EXPECT_NEAR(bboxs[1].x, 0.2, 1e-4);
    // EXPECT_NEAR(bboxs[1].y, 0.3, 1e-4);
    // EXPECT_NEAR(bboxs[1].w, 0.4, 1e-4);
    // EXPECT_NEAR(bboxs[1].h, 0.5, 1e-4);
    // EXPECT_NEAR(bboxs[1].score, 0.87, 1e-4);
    // EXPECT_EQ(bboxs[1].target, 1);
    EXPECT_EQ(decoder.end(), MA_OK);
}


}  // namespace ma
