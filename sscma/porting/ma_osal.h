#ifndef _MA_OSAL_H_
#define _MA_OSAL_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>

#include "core/ma_common.h"

#if __has_include(<ma_config_board.h>)
#include <ma_config_board.h>
#else
#error "No <ma_config_board.h>"
#endif

#if MA_OSAL_PTHREAD
#include "osal/ma_osal_pthread.h"
#elif MA_OSAL_FREERTOS
#include "osal/ma_osal_freertos.h"
#else
#error "No OSAL defined"
#endif

namespace ma {

class Tick {
public:
    static ma_tick_t current();
    static ma_tick_t fromMicroseconds(uint32_t us);
    static ma_tick_t fromMilliseconds(uint32_t ms);
    static ma_tick_t fromSeconds(uint32_t sec);
    static uint32_t toMicroseconds(ma_tick_t tick);
    static uint32_t toMilliseconds(ma_tick_t tick);
    static uint32_t toSeconds(ma_tick_t tick);
    static const ma_tick_t waitForever = MA_WAIT_FOREVER;
};

class Thread {
public:
    Thread(const char* name, void (*entry)(void*), void* arg = nullptr, uint32_t priority = 0, size_t stacksize = 0, ma_stack_t* stack = nullptr) noexcept;
    virtual ~Thread() noexcept;
    operator bool() const;
    bool start(void* arg = nullptr);
    bool stop();
    bool join();
    bool operator==(const Thread& other) const;

public:
    static void sleep(ma_tick_t tick);
    static void yield();
    static ma_thread_t* self();
    static void enterCritical();
    static void exitCritical();

protected:
    virtual void threadEntryPoint(void);

private:
    Thread(const Thread&)            = delete;
    Thread& operator=(const Thread&) = delete;

private:
#if MA_OSAL_PTHREAD
    static void* threadEntryPointStub(void* arg);
#elif MA_OSAL_FREERTOS
    static void threadEntryPointStub(void* arg);
#endif
    mutable ma_thread_t m_thread;
    void* m_arg;
    std::string m_name;
    void (*m_entry)(void*);
    uint32_t m_priority;
    size_t m_stackSize;
    ma_stack_t* m_stack;
    bool m_started;
};

class Mutex {
public:
    Mutex(bool recursive = true) noexcept;
    ~Mutex() noexcept;
    operator bool() const;
    bool tryLock(ma_tick_t timeout = Tick::waitForever);
    bool lock() const;
    bool unlock() const;
    operator ma_mutex_t*() const;

private:
    Mutex(const Mutex&)            = delete;
    Mutex& operator=(const Mutex&) = delete;

private:
    mutable ma_mutex_t m_mutex;
};

class Guard {
public:
    explicit Guard(const Mutex& mutex) noexcept;
    ~Guard() noexcept;

    operator bool() const;

private:
    const Mutex& m_mutex;
};

class Semaphore {
public:
    explicit Semaphore(size_t count = 0) noexcept;
    ~Semaphore() noexcept;
    operator bool() const;
    bool wait(ma_tick_t timeout = Tick::waitForever);
    uint32_t getCount() const;
    void signal();

private:
    Semaphore(const Semaphore&)            = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    Mutex m_mutex;
    ma_sem_t m_sem;
};

class Event {
public:
    Event() noexcept;
    ~Event() noexcept;
    operator bool() const;
    bool wait(uint32_t mask, uint32_t* value, ma_tick_t timeout = Tick::waitForever, bool clear = true, bool waitAll = false);
    void clear(uint32_t value = 0xffffffffu);
    void set(uint32_t value);
    uint32_t get() const;

private:
    Event(const Event&)            = delete;
    Event& operator=(const Event&) = delete;

private:
    Mutex m_mutex;
    ma_event_t m_event;
};

class MessageBox {
public:
    explicit MessageBox(size_t size = 1) noexcept;
    ~MessageBox() noexcept;
    operator bool() const;

    bool fetch(void** msg, ma_tick_t timeout = Tick::waitForever);
    bool post(void* msg, ma_tick_t timeout = Tick::waitForever);

private:
    MessageBox(const MessageBox&)            = delete;
    MessageBox& operator=(const MessageBox&) = delete;

private:
    Mutex m_mutex;
    ma_mbox_t m_mbox;
};

class Timer {
public:
    Timer(uint32_t ms, void (*fn)(ma_timer_t*, void*), void* arg, bool oneshot = false) noexcept;
    ~Timer() noexcept;
    operator bool() const;
    void set(ma_tick_t ms = Tick::waitForever);
    void start();
    void stop();

private:
    Timer(const Timer&)            = delete;
    Timer& operator=(const Timer&) = delete;

private:
    ma_timer_t m_timer;
};

class Signal {
public:
    static void install(std::vector<int> sigs, std::function<void(int sig)> callback);

private:
    static void sigHandler(int sig);

private:
    static std::map<int, std::vector<std::function<void(int sig)>>> s_callbacks;
    static Mutex m_mutex;
};


}  // namespace ma

#endif  // _MA_OSAL_H_