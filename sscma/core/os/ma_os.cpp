#include "ma_os.h"

namespace ma::os {

const char TAG[] = "ma::os";

// Tick class implementations
ma_tick_t Tick::current() {
    return ma_tick_current();
}

ma_tick_t Tick::fromMicroseconds(uint32_t us) {
    return ma_tick_from_us(us);
}

void Tick::sleep(ma_tick_t tick) {
    ma_tick_sleep(tick);
}

// Thread class implementations
Thread::Thread(const char* name,
               uint32_t    priority,
               size_t      stacksize,
               void (*entry)(void*),
               void* arg) noexcept {
    thread_ = ma_thread_create(name, priority, stacksize, entry, arg);
}

Thread::~Thread() noexcept {
    if (thread_) {
        ma_thread_delete(thread_);
    }
}

Thread::operator bool() const {
    return thread_ != nullptr;
}

void Thread::join() {
    if (thread_) {
        ma_thread_join(thread_);
        ma_thread_delete(thread_);
        thread_ = nullptr;
    }
}

// Mutex class implementations
Mutex::Mutex() noexcept {
    mutex_ = ma_mutex_create();
}

Mutex::~Mutex() noexcept {
    if (mutex_) {
        ma_mutex_delete(mutex_);
    }
}

Mutex::operator bool() const {
    return mutex_ != nullptr;
}

void Mutex::lock() const {
    if (mutex_) {
        ma_mutex_lock(mutex_);
    }
}

void Mutex::unlock() const {
    if (mutex_) {
        ma_mutex_unlock(mutex_);
    }
}

Guard::Guard(const Mutex& mutex) noexcept : mutex_(mutex) {
    if (mutex_) {
        mutex_.lock();
    }
}

Guard::~Guard() {
    if (mutex_) {
        mutex_.unlock();
    }
}

Guard::operator bool() const {
    return mutex_;
}


// Semaphore class implementations
Semaphore::Semaphore(size_t count) noexcept {
    sem_ = ma_sem_create(count);
}

Semaphore::~Semaphore() noexcept {
    if (sem_) {
        ma_sem_delete(sem_);
    }
}

Semaphore::operator bool() const {
    return sem_ != nullptr;
}

bool Semaphore::wait(uint32_t timeout) {
    return sem_ && ma_sem_wait(sem_, timeout);
}

void Semaphore::signal() {
    if (sem_) {
        ma_sem_signal(sem_);
    }
}

// Event class implementations
Event::Event() noexcept {
    event_ = ma_event_create();
}

Event::~Event() noexcept {
    if (event_) {
        ma_event_delete(event_);
    }
}

Event::operator bool() const {
    return event_ != nullptr;
}

bool Event::wait(uint32_t mask, uint32_t* value, uint32_t timeout) {
    return event_ && ma_event_wait(event_, mask, value, timeout);
}

void Event::set(uint32_t value) {
    if (event_) {
        ma_event_set(event_, value);
    }
}

void Event::clear(uint32_t value) {
    if (event_) {
        ma_event_clear(event_, value);
    }
}

// MessageBox class implementations
MessageBox::MessageBox(size_t size) noexcept {
    mbox_ = ma_mbox_create(size);
}

MessageBox::~MessageBox() noexcept {
    if (mbox_) {
        ma_mbox_delete(mbox_);
    }
}

MessageBox::operator bool() const {
    return mbox_ != nullptr;
}

bool MessageBox::fetch(const void** msg, uint32_t timeout) {
    return mbox_ && ma_mbox_fetch(mbox_, msg, timeout);
}

bool MessageBox::post(const void* msg, uint32_t timeout) {
    return mbox_ && ma_mbox_post(mbox_, msg, timeout);
}

// Timer class implementations
Timer::Timer(uint32_t us, void (*fn)(ma_timer_t*, void*), void* arg, bool oneshot) noexcept {
    timer_ = ma_timer_create(us, fn, arg, oneshot);
}

Timer::~Timer() noexcept {
    if (timer_) {
        ma_timer_delete(timer_);
    }
}

Timer::operator bool() const {
    return timer_ != nullptr;
}

void Timer::set(uint32_t us) {
    if (timer_) {
        ma_timer_set(timer_, us);
    }
}

void Timer::start() {
    if (timer_) {
        ma_timer_start(timer_);
    }
}

void Timer::stop() {
    if (timer_) {
        ma_timer_stop(timer_);
    }
}

}  // namespace ma::os
