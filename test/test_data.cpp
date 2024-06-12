#include <gtest/gtest.h>

#include <sscma.h>

#define TAG "test::data"

namespace ma::data {

TEST(DATA, RingBufferConstructible) {
    RingBuffer rb(1);
    EXPECT_TRUE(rb);
}

TEST(DATA, RingBufferWriteable) {
    RingBuffer rb(1);
    const uint8_t c = 'a';
    EXPECT_EQ(rb.write(&c, 1), 1);
    EXPECT_EQ(rb.remaining(), 1);
}

TEST(DATA, RingBufferReadable) {
    RingBuffer rb(1);
    uint8_t       c = 'a';
    EXPECT_EQ(rb.write(&c, 1), 1);
    EXPECT_EQ(rb.read(&c, 1), 1);
    EXPECT_EQ(rb.remaining(), 0);
    EXPECT_EQ(c, 'a');
}

TEST(DATA, RingBufferOperator) {
    RingBuffer rb(1);
    uint8_t       c = 'a';
    EXPECT_EQ(rb << c, rb);
    EXPECT_EQ(rb[0], 'a');
}

}  // namespace ma::data
