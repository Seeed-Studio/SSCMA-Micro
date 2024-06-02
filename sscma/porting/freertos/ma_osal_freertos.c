#include "core/ma_common.h"

#if MA_PORTING_FREERTOS == 1

#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "porting/ma_osal.h"
#include "porting/freertos/ma_osal_freertos.h"

#define TAG             "ma.os.freertos"

#define MS_TO_TICKS(ms) ((ms == MA_WAIT_FOREVER) ? portMAX_DELAY : (ms) / portTICK_PERIOD_MS)


ma_thread_t* ma_thread_create(
    const char* name, uint32_t priority, size_t stacksize, void (*entry)(void* arg), void* arg) {
    TaskHandle_t xHandle = NULL;

    /* stacksize in freertos is not in bytes but in stack depth, it should be
     * divided by the stack width */
    configSTACK_DEPTH_TYPE stackdepth = stacksize / sizeof(configSTACK_DEPTH_TYPE);

    if (xTaskCreate(entry, name, stackdepth, arg, priority, &xHandle) == pdPASS) {
        return (ma_thread_t*)xHandle;
    } 

    return NULL;
}


void ma_thread_delete(ma_thread_t* thread) {

    vTaskDelete((TaskHandle_t)thread);
}


bool ma_thread_join(ma_thread_t* thread) {

    return true;
}

ma_mutex_t* ma_mutex_create(void) {
    SemaphoreHandle_t handle = xSemaphoreCreateRecursiveMutex();
    MA_ASSERT(handle != NULL);
    return (ma_mutex_t*)handle;
}

void ma_mutex_lock(ma_mutex_t* _mutex) {

    xSemaphoreTakeRecursive((SemaphoreHandle_t)_mutex, portMAX_DELAY);
}

void ma_mutex_unlock(ma_mutex_t* _mutex) {
    xSemaphoreGiveRecursive((SemaphoreHandle_t)_mutex);
}

void ma_mutex_delete(ma_mutex_t* _mutex) {

    vSemaphoreDelete((SemaphoreHandle_t)_mutex);
}

ma_sem_t* ma_sem_create(size_t count) {

    SemaphoreHandle_t handle = xSemaphoreCreateCounting(count, 0);
    MA_ASSERT(handle != NULL);
    return (ma_sem_t*)handle;
}

bool ma_sem_wait(ma_sem_t* sem, uint32_t timeout) {

    return (xSemaphoreTake((SemaphoreHandle_t)sem, MS_TO_TICKS(timeout)) == pdTRUE);
}

void ma_sem_signal(ma_sem_t* sem) {

    xSemaphoreGive((SemaphoreHandle_t)sem);
}

void ma_sem_delete(ma_sem_t* sem) {

    vSemaphoreDelete((SemaphoreHandle_t)sem);
}

ma_tick_t ma_tick_current(void) {

    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

ma_tick_t ma_tick_from_us(uint32_t us) {

    return us / portTICK_PERIOD_MS;
}

void ma_tick_sleep(ma_tick_t tick) {

    vTaskDelay(tick);
}

ma_event_t* ma_event_create(void) {
    EventGroupHandle_t handle = xEventGroupCreate();
    MA_ASSERT(handle != NULL);
    return (ma_event_t*)handle;
}

bool ma_event_wait(ma_event_t* event, uint32_t mask, uint32_t* value, uint32_t timeout) {

    *value = xEventGroupWaitBits(
        (EventGroupHandle_t)event, mask, pdFALSE, pdFALSE, MS_TO_TICKS(timeout));
    *value &= mask;
    return (*value == mask);
}


void ma_event_set(ma_event_t* event, uint32_t value) {

    xEventGroupSetBits((EventGroupHandle_t)event, value);
}

void ma_event_clear(ma_event_t* event, uint32_t value) {

    xEventGroupClearBits((EventGroupHandle_t)event, value);
}

void ma_event_delete(ma_event_t* event) {

    vEventGroupDelete((EventGroupHandle_t)event);
}

ma_mbox_t* ma_mbox_create(size_t size) {
    QueueHandle_t handle = xQueueCreate(size, sizeof(void*));
    MA_ASSERT(handle != NULL);
    return (ma_mbox_t*)handle;
}

bool ma_mbox_fetch(ma_mbox_t* mbox, const void** msg, uint32_t timeout) {

    return (xQueueReceive((QueueHandle_t)mbox, msg, MS_TO_TICKS(timeout)) == pdTRUE);
}

bool ma_mbox_post(ma_mbox_t* mbox, const void* msg, uint32_t timeout) {

    return (xQueueSend((QueueHandle_t)mbox, &msg, MS_TO_TICKS(timeout)) == pdTRUE);
}

void ma_mbox_delete(ma_mbox_t* mbox) {

    vQueueDelete((QueueHandle_t)mbox);
}


static void ma_timer_callback(TimerHandle_t xTimer) {
    ma_timer_t* timer = pvTimerGetTimerID(xTimer);

    if (timer->fn)
        timer->fn(timer, timer->arg);
}

ma_timer_t* ma_timer_create(uint32_t us,
                            void (*fn)(ma_timer_t*, void* arg),
                            void* arg,
                            bool  oneshot) {
    ma_timer_t* timer;

    timer = (ma_timer_t*)ma_malloc(sizeof(ma_timer_t));
    if (timer == NULL) {
        return NULL;
    }

    timer->fn      = fn;
    timer->arg     = arg;
    timer->oneshot = oneshot;
    snprintf(timer->name, 32, "ma_timer_%04x", (uint32_t)timer);
    timer->handle = xTimerCreate("ma_timer",
                                 MS_TO_TICKS(us),
                                 timer->oneshot ? pdFALSE : pdTRUE,
                                 (void*)timer,
                                 ma_timer_callback);
    if (timer->handle == NULL) {
        ma_free(timer);
        return NULL;
    }


    return timer;
}

void ma_timer_set(ma_timer_t* timer, uint32_t us) {

    xTimerChangePeriod(timer->handle, MS_TO_TICKS(us), 0);
}

void ma_timer_start(ma_timer_t* timer) {

    xTimerStart(timer->handle, 0);
}

void ma_timer_stop(ma_timer_t* timer) {

    xTimerStop(timer->handle, 0);
}

void ma_timer_delete(ma_timer_t* timer) {

    xTimerDelete(timer->handle, 0);

    ma_free(timer);
}


#endif