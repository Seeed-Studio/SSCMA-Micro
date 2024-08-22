#include <gtest/gtest.h>

#include <sscma.h>

#define TAG "test::pipeline"

namespace ma {


TEST(PIPELINE, Executor) {
    static bool isRuned{false};
    static auto executor{Executor(1024 * 1024 * 4, 1)};
    executor.submit([](const std::atomic<bool>&) { isRuned = true; });
    Thread::sleep(Tick::fromSeconds(1));
    EXPECT_TRUE(isRuned);
}

}  // namespace ma
