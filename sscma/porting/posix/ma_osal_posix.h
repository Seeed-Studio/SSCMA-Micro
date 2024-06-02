#ifndef _MA_OSAL_POSIX_H_
#define _MA_OSAL_POSIX_H_

#include "core/ma_common.h"


#if MA_PORTING_POSIX == 1

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <time.h>

#include "porting/posix/ma_misc_posix.h"

#define MA_WAIT_FOREVER 0xFFFFFFFF

typedef pthread_t       ma_thread_t;
typedef pthread_mutex_t ma_mutex_t;

typedef struct ma_sem {
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    size_t          count;
} ma_sem_t;

typedef struct ma_event {
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    uint32_t        flags;
} ma_event_t;

typedef struct ma_mbox {
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    size_t          r;
    size_t          w;
    size_t          count;
    size_t          size;
    const void*     msg[];
} ma_mbox_t;

typedef struct ma_timer {
    timer_t      timerid;
    ma_thread_t* thread;
    pid_t        thread_id;
    bool         exit;
    void (*fn)(struct ma_timer*, void* arg);
    void*    arg;
    uint32_t us;
    bool     oneshot;
} ma_timer_t;

typedef uint64_t ma_tick_t;

#ifdef __cplusplus
}
#endif

#endif

#endif /* OSAL_POSIX_H */
