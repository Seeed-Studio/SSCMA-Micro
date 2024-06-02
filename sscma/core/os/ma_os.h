#ifndef _MA_OS_H_
#define _MA_OS_H_

#include <cstddef>
#include <cstdint>

#include "porting/ma_osal.h"

namespace ma::os {

class Tick {
public:
    static ma_tick_t      current();
    static ma_tick_t      fromMicroseconds(uint32_t us);
    static void           sleep(ma_tick_t tick);
    static const uint32_t waitForever = MA_WAIT_FOREVER;
};

class Thread {
public:
    Thread(const char* name,
           uint32_t    priority,
           size_t      stacksize,
           void (*entry)(void*),
           void* arg = nullptr) noexcept;
    ~Thread() noexcept;
    operator bool() const;
    void join();

private:
    mutable ma_thread_t* thread_;
};

class Mutex {
public:
    Mutex() noexcept;
    ~Mutex() noexcept;
    operator bool() const;
    void lock() const;
    void unlock() const;

private:
    mutable ma_mutex_t* mutex_;
};

class Guard {
public:
    explicit Guard(const Mutex& mutex) noexcept;
    ~Guard() noexcept;

    Guard(const Guard&)            = delete;
    Guard& operator=(const Guard&) = delete;

    operator bool() const;

private:
    const Mutex& mutex_;
};

class Semaphore {
public:
    explicit Semaphore(size_t count = 1) noexcept;
    ~Semaphore() noexcept;
    operator bool() const;
    bool wait(uint32_t timeout = Tick::waitForever);
    void signal();

private:
    mutable ma_sem_t* sem_;
};

class Event {
public:
    Event() noexcept;
    ~Event() noexcept;
    operator bool() const;
    bool wait(uint32_t mask, uint32_t* value, uint32_t timeout = Tick::waitForever);
    void set(uint32_t value);
    void clear(uint32_t value);

private:
    mutable ma_event_t* event_;
};

class MessageBox {
public:
    explicit MessageBox(size_t size = 1) noexcept;
    ~MessageBox() noexcept;
    operator bool() const;
    bool fetch(const void** msg, uint32_t timeout = Tick::waitForever);
    bool post(const void* msg, uint32_t timeout = Tick::waitForever);

private:
    mutable ma_mbox_t* mbox_;
};

class Timer {
public:
    Timer(uint32_t us, void (*fn)(ma_timer_t*, void*), void* arg, bool oneshot = false) noexcept;
    ~Timer() noexcept;
    operator bool() const;
    void set(uint32_t us = Tick::waitForever);
    void start();
    void stop();

private:
    mutable ma_timer_t* timer_;
};


}  // namespace ma::os

#endif  // _MA_OS_H_
