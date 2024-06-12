#include <gtest/gtest.h>

#include <sscma.h>

namespace ma {


TEST(CodecTest, CodecJSONTEST) {
    CodecJSON codec;
    EXPECT_EQ(codec.begin(MA_REPLY_RESPONSE, MA_OK, "TEST"), MA_OK);
    EXPECT_EQ(codec.write("bool", true), MA_OK);
    EXPECT_EQ(codec.write("i8", static_cast<int8_t>(1)), MA_OK);
    EXPECT_EQ(codec.write("i16", static_cast<int16_t>(2)), MA_OK);
    EXPECT_EQ(codec.write("i32", static_cast<int32_t>(3)), MA_OK);
    EXPECT_EQ(codec.write("i64", static_cast<int64_t>(4)), MA_OK);
    EXPECT_EQ(codec.write("u8", static_cast<uint8_t>(5)), MA_OK);
    EXPECT_EQ(codec.write("u16", static_cast<uint16_t>(6)), MA_OK);
    EXPECT_EQ(codec.write("u32", static_cast<uint32_t>(7)), MA_OK);
    EXPECT_EQ(codec.write("u64", static_cast<uint64_t>(8)), MA_OK);
    EXPECT_EQ(codec.write("float", static_cast<float>(9.5f)), MA_OK);
    EXPECT_EQ(codec.write("double", static_cast<double>(10.5f)), MA_OK);
    EXPECT_EQ(codec.write("string", std::string("test")), MA_OK);
    EXPECT_EQ(codec.write("string", std::string("test2")), MA_EEXIST);
    const ma_perf_t perf = {11, 12, 13};
    EXPECT_EQ(codec.write(perf), MA_OK);
    EXPECT_EQ(codec.end(), MA_OK);
    printf("%s\n", codec.toString().c_str());
    EXPECT_TRUE(static_cast<bool>(codec));
}

}  // namespace ma
