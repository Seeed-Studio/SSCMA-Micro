#ifndef _MA_OSAL_H_
#define _MA_OSAL_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "core/ma_compiler.h"
#include "core/ma_config_internal.h"
#if MA_PORTING_POSIX == 1
#include "porting/posix/ma_osal_posix.h"
#elif MA_PORTING_FREERTOS == 1
#include "porting/freertos/ma_osal_freertos.h"
#else
#error "Unsupported platform"
#endif


/**
 * @brief Get the current tick
 *
 * @return The current tick value
 */
ma_tick_t ma_tick_current(void);

/**
 * @brief Get the tick value from the given microseconds
 *
 * @param us The time in microseconds
 * @return The tick value corresponding to the given time
 */
ma_tick_t ma_tick_from_us(uint32_t us);

/**
 * @brief Sleep until the given tick value
 *
 * @param tick The tick value to sleep until
 */
void ma_tick_sleep(ma_tick_t tick);


/**
 * @brief Create a thread
 *
 * @param name Thread name
 * @param priority Thread priority
 * @param stacksize Thread stack size
 * @param entry Thread entry function
 * @param arg Thread argument
 * @return Pointer to the thread or NULL if creation failed
 */
ma_thread_t* ma_thread_create(
    const char* name, uint32_t priority, size_t stacksize, void (*entry)(void* arg), void* arg);

/**
 * @brief Delete a thread
 *
 * @param thread Pointer to the thread
 * @return MA_OK if the thread was deleted
 */
void ma_thread_delete(ma_thread_t* thread);

/**
 * @brief Join a thread
 *
 * @param thread Pointer to the thread
 * @return True if the thread was joined
 */
bool ma_thread_join(ma_thread_t* thread);

/**
 * @brief Create a mutex
 *
 * @return Pointer to the mutex or NULL if creation failed
 */
ma_mutex_t* ma_mutex_create(void);

/**
 * @brief Lock a mutex
 *
 * @param mutex Pointer to the mutex
 */
void ma_mutex_lock(ma_mutex_t* mutex);

/**
 * @brief Unlock a mutex
 *
 * @param mutex Pointer to the mutex
 */
void ma_mutex_unlock(ma_mutex_t* mutex);

/**
 * @brief Delete a mutex
 *
 * @param mutex Pointer to the mutex
 */
void ma_mutex_delete(ma_mutex_t* mutex);

/**
 * @brief Create a semaphore
 *
 * @param count Initial value of the semaphore
 * @return Pointer to the semaphore or NULL if creation failed
 */
ma_sem_t* ma_sem_create(size_t count);

/**
 * @brief Wait for a semaphore
 *
 * @param sem Pointer to the semaphore
 * @param timeout Time to wait in milliseconds
 * @return True if the semaphore was signaled, false if timeout
 */
bool ma_sem_wait(ma_sem_t* sem, uint32_t timeout);

/**
 * @brief Signal a semaphore
 *
 * @param sem Pointer to the semaphore
 */
void ma_sem_signal(ma_sem_t* sem);

/**
 * @brief Delete a semaphore
 *
 * @param sem Pointer to the semaphore
 */
void ma_sem_delete(ma_sem_t* sem);

/**
 * @brief Create an event
 *
 * @return Pointer to the event or NULL if creation failed
 */
ma_event_t* ma_event_create(void);

/**
 * @brief Wait for an event
 *
 * @param event Pointer to the event
 * @param mask Mask of the bits in the event to wait for
 * @param value Pointer to the value to wait for
 * @param timeout Time to wait in milliseconds
 * @return True if the event was signaled, false if timeout
 */
bool ma_event_wait(ma_event_t* event, uint32_t mask, uint32_t* value, uint32_t timeout);

/**
 * @brief Set an event
 *
 * @param event Pointer to the event
 * @param value Value to set in the event
 */
void ma_event_set(ma_event_t* event, uint32_t value);

/**
 * @brief Clear an event
 *
 * @param event Pointer to the event
 * @param value Value to clear in the event
 */
void ma_event_clear(ma_event_t* event, uint32_t value);

/**
 * @brief Delete an event
 *
 * @param event Pointer to the event
 */
void ma_event_delete(ma_event_t* event);

/**
 * @brief Create a message box
 *
 * @param size Size of the message box
 * @return Pointer to the message box or NULL if creation failed
 */
ma_mbox_t* ma_mbox_create(size_t size);

/**
 * @brief Fetch a message from a message box
 *
 * @param mbox Pointer to the message box
 * @param msg Pointer to the message
 * @param timeout Time to wait in milliseconds
 * @return True if a message was fetched, false if timeout
 */
bool ma_mbox_fetch(ma_mbox_t* mbox, const void** msg, uint32_t timeout);

/**
 * @brief Post a message to a message box
 *
 * @param mbox Pointer to the message box
 * @param msg Message to post
 * @param timeout Time to wait in milliseconds
 * @return True if the message was posted, false if timeout
 */
bool ma_mbox_post(ma_mbox_t* mbox, const void* msg, uint32_t timeout);

/**
 * @brief Delete a message box
 *
 * @param mbox Pointer to the message box
 */
void ma_mbox_delete(ma_mbox_t* mbox);

/**
 * @brief Create a timer
 *
 * @param us Period of the timer in microseconds
 * @param fn Timer callback function
 * @param arg Timer callback argument
 * @param oneshot If true, the timer will trigger immediately
 * @return Pointer to the timer or NULL if creation failed
 */
ma_timer_t* ma_timer_create(uint32_t us,
                            void (*fn)(ma_timer_t* timer, void* arg),
                            void* arg,
                            bool  oneshot);

/**
 * @brief Set the period of a timer
 *
 * @param timer Pointer to the timer
 * @param us Period of the timer in microseconds
 */
void ma_timer_set(ma_timer_t* timer, uint32_t us);

/**
 * @brief Start a timer
 *
 * @param timer Pointer to the timer
 */
void ma_timer_start(ma_timer_t* timer);

/**
 * @brief Stop a timer
 *
 * @param timer Pointer to the timer
 */
void ma_timer_stop(ma_timer_t* timer);

/**
 * @brief Delete a timer
 *
 * @param timer Pointer to the timer
 */
void ma_timer_delete(ma_timer_t* timer);


#ifdef __cplusplus
}
#endif

#endif /* _MA_OSAL_H_ */
