#include <gtest/gtest.h>

#include <sscma.h>


#define TAG "test::data"

namespace ma {

TEST(DATA, RingBufferConstructible) {
    ring_buffer<uint8_t> rb(1);
    EXPECT_TRUE(rb);
}

TEST(DATA, RingBufferWriteable) {
    ring_buffer<uint8_t> rb(1);
    const uint8_t c = 'a';
    EXPECT_EQ(rb.write(&c, 1), 1);
    EXPECT_EQ(rb.size(), 1);
}

TEST(DATA, RingBufferReadable) {
    ring_buffer<uint8_t> rb(1);
    uint8_t c = 'a';
    EXPECT_EQ(rb.write(&c, 1), 1);
    EXPECT_EQ(rb.read(&c, 1), 1);
    EXPECT_EQ(rb.size(), 0);
    EXPECT_EQ(c, 'a');
}

TEST(DATA, RingBufferOperator) {
    ring_buffer<uint8_t> rb(1);
    uint8_t c = 'a';
    EXPECT_EQ(rb << c, rb);
    EXPECT_EQ(rb[0], 'a');
}

}  // namespace ma
