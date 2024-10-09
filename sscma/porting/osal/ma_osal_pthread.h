#ifndef _MA_OSAL_PTHREAD_H_
#define _MA_OSAL_PTHREAD_H_

#include "core/ma_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <csignal>
#include <pthread.h>
#include <time.h>

#define MA_WAIT_FOREVER 0xFFFFFFFF

typedef uint64_t ma_tick_t;
typedef pthread_t ma_thread_t;
typedef pthread_mutex_t ma_mutex_t;
typedef void* ma_stack_t;

typedef struct ma_sem {
    pthread_cond_t cond;
    size_t count;
} ma_sem_t;

typedef struct ma_event {
    pthread_cond_t cond;
    uint32_t value;
} ma_event_t;

typedef struct ma_mbox {
    pthread_cond_t cond;
    size_t r;
    size_t w;
    size_t count;
    size_t size;
    void* msg[64];
} ma_mbox_t;

typedef struct ma_timer {
    timer_t timerid;
    bool exit;
    void (*fn)(struct ma_timer*, void* arg);
    void* arg;
    uint32_t ms;
    bool oneshot;
} ma_timer_t;


#ifdef __cplusplus
}
#endif

#endif  // _MA_OSAL_PTHREAD_H_