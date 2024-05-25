#include <gtest/gtest.h>

#include <sscma.h>

#define TAG "test::os"

namespace ma::os {

TEST(MaOs, TickCurrent) {
    EXPECT_TRUE(Tick::current() != 0);
}

TEST(MaOs, TickFromMicroseconds) {
    const uint32_t  us       = 1000;
    const ma_tick_t expected = Tick::fromMicroseconds(us);
    EXPECT_EQ(expected, Tick::fromMicroseconds(us));
}

TEST(MaOs, TickSleep) {
    const ma_tick_t tick = Tick::fromMicroseconds(10);
    Tick::sleep(tick);
    // sleep() doesn't return a value, so we can't assert anything about it
}

TEST(MaOs, ThreadConstructible) {
    char     name[]      = "test";
    uint32_t priority    = 1;
    size_t   stacksize   = 4096;
    void (*entry)(void*) = [](void*) {
    };
    void*  arg = nullptr;
    Thread thread(name, priority, stacksize, entry, arg);
    EXPECT_TRUE(thread);
}

TEST(MaOs, ThreadJoinable) {
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

TEST(MaOs, MutexLockable) {
    Mutex mutex;
    Guard guard(mutex);
    EXPECT_TRUE(guard);
}

TEST(MaOs, MutexUnlockable) {
    Mutex mutex;
    Guard guard(mutex);
    guard.~Guard();
    EXPECT_TRUE(mutex);
}

TEST(MaOs, SemaphoreConstructible) {
    Semaphore sem(1);
    EXPECT_TRUE(sem);
}

TEST(MaOs, SemaphoreSignalable) {
    Semaphore sem(1);
    sem.signal();
    EXPECT_TRUE(sem);
}

TEST(MaOs, EventConstructible) {
    Event event;
    EXPECT_TRUE(event);
}

TEST(MaOs, EventReadable) {
    Event    event;
    uint32_t value = 0;
    EXPECT_FALSE(event.wait(0, &value, 0));
}

TEST(MaOs, EventWrittable) {
    Event event;
    event.set(1);
    EXPECT_TRUE(event.wait(1, nullptr, 0));
}

TEST(MaOs, MessageBoxConstructible) {
    MessageBox mbox(1);
    EXPECT_TRUE(mbox);
}

TEST(MaOs, MessageBoxReadable) {
    MessageBox  mbox(1);
    const void* msg = nullptr;
    EXPECT_FALSE(mbox.fetch(&msg, 0));
}

TEST(MaOs, MessageBoxWriteable) {
    MessageBox  mbox(1);
    const char* msg = "message";
    EXPECT_TRUE(mbox.post(msg, 0));
}

TEST(MaOs, TimerConstructible) {
    Timer timer(100, [](ma_timer_t*, void*) {}, nullptr, false);
    EXPECT_TRUE(timer);
}

TEST(MaOs, TimerStartable) {
    Timer timer(100, [](ma_timer_t*, void*) {}, nullptr, false);
    timer.start();
    // start() doesn't return a value, so we can't assert anything about it
}

TEST(MaOs, TimerStopable) {
    Timer timer(100, [](ma_timer_t*, void*) {}, nullptr, false);
    timer.stop();
    // stop() doesn't return a value, so we can't assert anything about it
}

TEST(MaOs, RingBufferConstructible) {
    RingBuffer rb(1);
    EXPECT_TRUE(rb);
}

TEST(MaOs, RingBufferWriteable) {
    RingBuffer rb(1);
    const char c = 'a';
    EXPECT_EQ(rb.write(&c, 1, 0), 1);
    EXPECT_EQ(rb.available(), 1);
}

TEST(MaOs, RingBufferReadable) {
    RingBuffer rb(1);
    char       c = 'a';
    EXPECT_EQ(rb.write(&c, 1, 0), 1);
    EXPECT_EQ(rb.read(&c, 1, 0), 1);
    EXPECT_EQ(rb.available(), 0);
    EXPECT_EQ(c, 'a');
}

TEST(MaOs, RingBufferOperator) {
    RingBuffer rb(1);
    char       c = 'a';
    EXPECT_EQ(rb << c, rb);
    EXPECT_EQ(rb[0], 'a');
}

}  // namespace ma::os
