
#include <gtest/gtest.h>

#include <sscma.h>

namespace ma {

// Test for Mutex
TEST(MutexTest, LockUnlock) {
    Mutex mutex;
    EXPECT_TRUE(mutex.lock());
    EXPECT_TRUE(mutex.unlock());
}

TEST(MutexTest, TryLock) {
    Mutex mutex;
    EXPECT_TRUE(mutex.tryLock());
    EXPECT_TRUE(mutex.unlock());
}

// Test for Semaphore
TEST(SemaphoreTest, SignalWait) {
    Semaphore sem(1);
    EXPECT_TRUE(sem.wait());
    sem.signal();
    EXPECT_EQ(sem.getCount(), 1);
}

// // Test for Event
// TEST(EventTest, SetClearWait) {
//     Event event;
//     uint32_t value;
//     event.set(0x01);
//     EXPECT_TRUE(event.wait(0x01, &value));
//     EXPECT_EQ(value, 0x01);
//     event.clear(0x01);
//     EXPECT_FALSE(event.wait(0x01, &value, 0));
// }

// // Test for Timer
// void timerCallback(ma_timer_t*, void*) {
//     // Timer callback implementation
// }

// TEST(TimerTest, StartStop) {
//     Timer timer(1000, timerCallback, nullptr);
//     EXPECT_TRUE(static_cast<bool>(timer));
//     timer.start();
//     timer.stop();
// }

// Test for Thread
static bool entryCalled = false;
void threadEntry(void*) {
    // Thread entry function
    while (1) {
        entryCalled = true;
        printf("Hello from thread!\n");
        Tick::sleep(Tick::fromMilliseconds(100));
    }
}

TEST(ThreadTest, Start) {
    Thread thread("TestThread", threadEntry);
    thread.start();
    EXPECT_TRUE(static_cast<bool>(thread));
    sleep(1);
    EXPECT_TRUE(entryCalled);
}

}  // namespace ma