#include <gtest/gtest.h>

#include <sscma.h>

#include "core/pipeline/executor.hpp"


#define TAG "test::pipeline"

namespace ma {


TEST(PIPELINE, Executor) {
    static bool isRuned{false};
    static auto executor{Executor(1024 * 1024 * 4, 1)};
    executor.add_task([](const std::atomic<bool>&) { isRuned = true; });
    Tick::sleep(Tick::fromSeconds(1));
    EXPECT_TRUE(isRuned);
}

}  // namespace ma
