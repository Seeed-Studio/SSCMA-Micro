#include <gtest/gtest.h>

#include <sscma.h>

#define TAG "test::os"

namespace ma::os {

TEST(OS, TickCurrent) {
    EXPECT_TRUE(Tick::current() != 0);
}

TEST(OS, TickFromMicroseconds) {
    const uint32_t  us       = 1000;
    const ma_tick_t expected = Tick::fromMicroseconds(us);
    EXPECT_EQ(expected, Tick::fromMicroseconds(us));
}

TEST(OS, TickSleep) {
    const ma_tick_t tick = Tick::fromMicroseconds(10);
    Tick::sleep(tick);
    // sleep() doesn't return a value, so we can't assert anything about it
}

TEST(OS, ThreadConstructible) {
    char     name[]      = "test";
    uint32_t priority    = 1;
    size_t   stacksize   = 4096;
    void (*entry)(void*) = [](void*) {
    };
    void*  arg = nullptr;
    Thread thread(name, priority, stacksize, entry, arg);
    EXPECT_TRUE(thread);
}

TEST(OS, ThreadJoinable) {
    char     name[]      = "test";
    uint32_t priority    = 1;
    size_t   stacksize   = 4096;
    void (*entry)(void*) = [](void*) {
    };
    void*  arg = nullptr;
    Thread thread(name, priority, stacksize, entry, arg);
    thread.join();
    EXPECT_FALSE(thread);
}

TEST(OS, MutexLockable) {
    Mutex mutex;
    Guard guard(mutex);
    EXPECT_TRUE(guard);
}

TEST(OS, MutexUnlockable) {
    Mutex mutex;
    Guard guard(mutex);
    guard.~Guard();
    EXPECT_TRUE(mutex);
}

TEST(OS, SemaphoreConstructible) {
    Semaphore sem(1);
    EXPECT_TRUE(sem);
}

TEST(OS, SemaphoreSignalable) {
    Semaphore sem(1);
    sem.signal();
    EXPECT_TRUE(sem);
}

TEST(OS, EventConstructible) {
    Event event;
    EXPECT_TRUE(event);
}

TEST(OS, EventReadable) {
    Event    event;
    uint32_t value = 0;
    EXPECT_FALSE(event.wait(0, &value, 0));
}

TEST(OS, EventWrittable) {
    Event event;
    event.set(1);
    EXPECT_TRUE(event.wait(1, nullptr, 0));
}

TEST(OS, MessageBoxConstructible) {
    MessageBox mbox(1);
    EXPECT_TRUE(mbox);
}

TEST(OS, MessageBoxReadable) {
    MessageBox  mbox(1);
    const void* msg = nullptr;
    EXPECT_FALSE(mbox.fetch(&msg, 0));
}

TEST(OS, MessageBoxWriteable) {
    MessageBox  mbox(1);
    const char* msg = "message";
    EXPECT_TRUE(mbox.post(msg, 0));
}

TEST(OS, TimerConstructible) {
    Timer timer(100, [](ma_timer_t*, void*) {}, nullptr, false);
    EXPECT_TRUE(timer);
}

TEST(OS, TimerStartable) {
    Timer timer(100, [](ma_timer_t*, void*) {}, nullptr, false);
    timer.start();
    // start() doesn't return a value, so we can't assert anything about it
}

TEST(OS, TimerStopable) {
    Timer timer(100, [](ma_timer_t*, void*) {}, nullptr, false);
    timer.stop();
    // stop() doesn't return a value, so we can't assert anything about it
}

}  // namespace ma::os
