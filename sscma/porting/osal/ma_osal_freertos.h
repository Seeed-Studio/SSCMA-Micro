#ifndef _MA_OSAL_FREERTOS_H_
#define _MA_OSAL_FREERTOS_H_

#include "core/ma_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#define MA_WAIT_FOREVER portMAX_DELAY

typedef TickType_t ma_tick_t;
typedef SemaphoreHandle_t ma_mutex_t;
typedef TaskHandle_t ma_thread_t;
typedef SemaphoreHandle_t ma_sem_t;
typedef EventGroupHandle_t ma_event_t;
typedef QueueHandle_t ma_mbox_t;
typedef StackType_t ma_stack_t;

typedef struct ma_timer {
    TimerHandle_t handle;
    void (*fn)(struct ma_timer*, void* arg);
    void* arg;
    uint32_t us;
    bool oneshot;
    char name[16];
} ma_timer_t;

#ifdef __cplusplus
}
#endif

#endif  // _MA_OSAL_FREERTOS_H_