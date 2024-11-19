#include "core/ma_common.h"
#include "porting/ma_osal.h"


#if MA_OSAL_FREERTOS

#ifndef MA_OSAL_FREERTOS_CXX_ALLOC_OVERRIDE
#define MA_OSAL_FREERTOS_CXX_ALLOC_OVERRIDE 1
#endif

#if MA_OSAL_FREERTOS_CXX_ALLOC_OVERRIDE
MA_ATTR_WEAK void* operator new(size_t size) {
    // printf("new size: %d\n", size);
    void* ptr = pvPortMalloc(size);
    MA_ASSERT(ptr!= nullptr);
    return ptr;
}

MA_ATTR_WEAK void* operator new[](size_t size) {
    // printf("new size: %d\n", size);
    void* ptr = pvPortMalloc(size);
    MA_ASSERT(ptr!= nullptr);
    return ptr;
}

MA_ATTR_WEAK void operator delete(void* ptr) {
    MA_ASSERT(ptr!= nullptr);
    vPortFree(ptr);
}

MA_ATTR_WEAK void operator delete[](void* ptr) {
    MA_ASSERT(ptr!= nullptr);
    vPortFree(ptr);
}
#endif

#ifndef MA_OSAL_RTOS_EXTERN_PII
#define MA_OSAL_RTOS_EXTERN_PII 0
#endif

#if MA_OSAL_RTOS_EXTERN_PII
extern "C" { BaseType_t xPortInIsrContext(void); }
#else
#define xPortIsInsideInterrupt() (xPortInIsrContext())
#endif

namespace ma {

    #define MS_TO_TICKS(ms) ((ms == Tick::waitForever) ? portMAX_DELAY : (ms) / portTICK_PERIOD_MS)

ma_tick_t Tick::current() { return xTaskGetTickCount() * portTICK_PERIOD_MS; }

ma_tick_t Tick::fromMicroseconds(uint32_t us) { return (us / 1000) / portTICK_PERIOD_MS; }

ma_tick_t Tick::fromMilliseconds(uint32_t ms) { return ms / portTICK_PERIOD_MS; }

ma_tick_t Tick::fromSeconds(uint32_t sec) { return (sec * 1000) / portTICK_PERIOD_MS; }

void Thread::sleep(ma_tick_t tick) { vTaskDelay(tick); }

Thread::Thread(const char* name, void (*entry)(void*), void* arg, uint32_t priority, size_t stackSize, ma_stack_t* stack) noexcept
    : m_thread(nullptr),
      m_arg(arg),
      m_name(name),
      m_entry(entry),
      m_priority(priority),
      m_stackSize(stackSize),
      m_stack(stack) {}

void Thread::threadEntryPoint(void) {
    if (m_entry != nullptr) {
        m_entry(m_arg ? m_arg : nullptr);
    }
}

void Thread::threadEntryPointStub(void* arg) {
    Thread* thread = static_cast<Thread*>(arg);
    if (thread != nullptr) {
        thread->threadEntryPoint();
    }
    return;
}

Thread::~Thread() noexcept {}

Thread::operator bool() const { return m_thread != nullptr; }

bool Thread::start(void* arg) {
    if (arg) {
        m_arg = arg;
    }

    if (m_thread != nullptr) {
        return false;
    }

    auto err = xTaskCreate(threadEntryPointStub,
                    (m_name.empty() ? "Thread" : m_name.c_str()),
                    (configSTACK_DEPTH_TYPE)((m_stackSize + sizeof(uint32_t) - 1U) / sizeof(uint32_t)),
                    this,
                    m_priority,
                    &m_thread);

    if (err != pdPASS) {
        MA_LOGE("Thread", "Thread creation failed: %d", static_cast<int>(err));
        return false;
    }

    return true;
}

bool Thread::stop() {
    if (m_thread != nullptr) {
        vTaskDelete(m_thread);
        m_thread = nullptr;
    }
    return true;
}

bool Thread::operator==(const Thread& other) const { return m_thread == other.m_thread; }

void Thread::yield() { portYIELD(); }

ma_thread_t* Thread::self() { return reinterpret_cast<ma_thread_t*>(xTaskGetCurrentTaskHandle()); }

Mutex::Mutex(bool recursive) noexcept : m_mutex(nullptr) {
    if (recursive) {
        m_mutex = xSemaphoreCreateRecursiveMutex();
    } else {
        m_mutex = xSemaphoreCreateMutex();
    }
}

Mutex::operator ma_mutex_t*() const { return &m_mutex; }

Mutex::~Mutex() noexcept { vSemaphoreDelete(m_mutex); }

Mutex::operator bool() const { return m_mutex != nullptr; }

bool Mutex::tryLock(ma_tick_t timeout) {
    if (xPortIsInsideInterrupt()) {
        return false;
    }
    return (xSemaphoreTakeRecursive(m_mutex, 0) == pdTRUE);
}

bool Mutex::lock() const {
    if (xPortIsInsideInterrupt()) {
        return false;
    }
    return (xSemaphoreTakeRecursive(m_mutex, portMAX_DELAY) == pdTRUE);
}

bool Mutex::unlock() const {
    if (xPortIsInsideInterrupt()) {
        return false;
    }
    return (xSemaphoreGiveRecursive(m_mutex) == pdTRUE);
}

Guard::Guard(const Mutex& mutex) noexcept : m_mutex(mutex) { m_mutex.lock(); }

Guard::~Guard() noexcept { m_mutex.unlock(); }

Guard::operator bool() const { return m_mutex; }

Semaphore::Semaphore(size_t count) noexcept : m_sem(nullptr) {
    m_sem = xSemaphoreCreateCounting(0x7fffffffu, static_cast<UBaseType_t>(count));
}

Semaphore::~Semaphore() noexcept { vSemaphoreDelete(m_sem); }

Semaphore::operator bool() const { return m_sem != nullptr; }

bool Semaphore::wait(ma_tick_t timeout) {
    BaseType_t yield;
    if (xPortIsInsideInterrupt()) {
        if (xSemaphoreTakeFromISR(m_sem, &yield) != pdPASS) {
            return false;
        } else {
            portYIELD_FROM_ISR(yield);
        }
    } else {
        Guard guard(m_mutex);
        if (xSemaphoreTake(m_sem, MS_TO_TICKS(timeout)) != pdPASS) {
            return false;
        }
    }
    return true;
}

uint32_t Semaphore::getCount() const {
    if (xPortIsInsideInterrupt()) {
        return static_cast<uint32_t>(uxQueueMessagesWaitingFromISR(m_sem));
    } else {
        return static_cast<uint32_t>(uxSemaphoreGetCount(m_sem));
    }
}

void Semaphore::signal() {
    BaseType_t yield;
    if (xPortIsInsideInterrupt()) {
        if (xSemaphoreGiveFromISR(m_sem, &yield) != pdPASS) {
            return;
        } else {
            portYIELD_FROM_ISR(yield);
        }
    } else {
        xSemaphoreGive(m_sem);
    }
}

Event::Event() noexcept : m_event(nullptr) { m_event = xEventGroupCreate(); }

Event::~Event() noexcept { vEventGroupDelete(m_event); }

Event::operator bool() const { return m_event != nullptr; }

bool Event::wait(uint32_t mask, uint32_t* value, ma_tick_t timeout, bool clear, bool waitAll) {
    if (xPortIsInsideInterrupt()) {
        *value = 0;
        return false;
    }
    Guard guard(m_mutex);
    *value =
      xEventGroupWaitBits(m_event, mask, waitAll ? pdTRUE : pdFALSE, clear ? pdTRUE : pdFALSE, MS_TO_TICKS(timeout));
    *value &= mask;
    return (*value == mask);
}

void Event::clear(uint32_t value) {
    if (xPortIsInsideInterrupt()) {
        xEventGroupClearBitsFromISR(m_event, value);
    } else {
        Guard guard(m_mutex);
        xEventGroupClearBits(m_event, value);
    }
}

void Event::set(uint32_t value) {
    BaseType_t yield;
    if (xPortIsInsideInterrupt()) {
        if (xEventGroupSetBitsFromISR(m_event, value, &yield) == pdPASS) {
            portYIELD_FROM_ISR(yield);
        }
    } else {
        Guard guard(m_mutex);
        xEventGroupSetBits(m_event, value);
    }
}

uint32_t Event::get() const {
    if (xPortIsInsideInterrupt()) {
        return static_cast<uint32_t>(xEventGroupGetBitsFromISR(m_event));
    } else {
        Guard guard(m_mutex);
        return static_cast<uint32_t>(xEventGroupGetBits(m_event));
    }
}

MessageBox::MessageBox(size_t size) noexcept : m_mbox(nullptr) { m_mbox = xQueueCreate(size, sizeof(void*)); }

MessageBox::~MessageBox() noexcept { vQueueDelete(m_mbox); }

MessageBox::operator bool() const { return m_mbox != nullptr; }

bool MessageBox::fetch(void** msg, ma_tick_t timeout) {
    BaseType_t yield;
    if (xPortIsInsideInterrupt()) {
        if (xQueueReceiveFromISR(m_mbox, msg, &yield) != pdPASS) {
            return false;
        } else {
            portYIELD_FROM_ISR(yield);
        }
    } else {
        Guard guard(m_mutex);
        if (xQueueReceive(m_mbox, msg, MS_TO_TICKS(timeout)) != pdPASS) {
            return false;
        }
    }
    return true;
}

bool MessageBox::post(void* msg, ma_tick_t timeout) {
    BaseType_t yield;
    if (xPortIsInsideInterrupt()) {
        if (xQueueSendFromISR(m_mbox, &msg, &yield) != pdPASS) {
            return false;
        } else {
            portYIELD_FROM_ISR(yield);
        }
    } else {
        Guard guard(m_mutex);
        if (xQueueSend(m_mbox, &msg, MS_TO_TICKS(timeout)) != pdPASS) {
            return false;
        }
    }
    return true;
}

}  // namespace ma

#endif