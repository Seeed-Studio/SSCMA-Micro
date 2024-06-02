
#define _GNU_SOURCE /* For pthread_setname_mp() */

#include "core/ma_common.h"

#if MA_PORTING_POSIX == 1


#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <pthread.h>

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <sys/syscall.h>

#include "porting/ma_osal.h"
#include "porting/posix/ma_osal_posix.h"

#define TAG "ma.os.posix"


ma_thread_t* ma_thread_create(
    const char* name, uint32_t priority, size_t stacksize, void (*entry)(void* arg), void* arg) {
    int            result;
    pthread_t*     thread;
    pthread_attr_t attr;

    thread = ma_malloc(sizeof(*thread));
    if (thread == NULL) {
        return NULL;
    }

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN + stacksize);

    result = pthread_create(thread, &attr, (void*)entry, arg);
    if (result != 0) {
        ma_free(thread);
        return NULL;
    }

    pthread_setname_np(*thread, name);
    return thread;
}


void ma_thread_delete(ma_thread_t* thread) {
    ma_free(thread);
}


bool ma_thread_join(ma_thread_t* thread) {

    int result;
    result = pthread_join(*thread, NULL);
    return (result == 0);
}

ma_mutex_t* ma_mutex_create(void) {
    int                 result;
    pthread_mutex_t*    mutex;
    pthread_mutexattr_t mattr;

    mutex = ma_malloc(sizeof(*mutex));
    if (mutex == NULL) {
        return NULL;
    }

    // MA_STATIC_ASSERT(_POSIX_THREAD_PRIO_PROTECT > 0, "POSIX_THREAD_PRIO_PROTECT is not defined");

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);

    result = pthread_mutex_init(mutex, &mattr);
    if (result != 0) {
        ma_free(mutex);
        return NULL;
    }

    return mutex;
}

void ma_mutex_lock(ma_mutex_t* _mutex) {
    pthread_mutex_t* mutex = _mutex;
    pthread_mutex_lock(mutex);
}

void ma_mutex_unlock(ma_mutex_t* _mutex) {
    pthread_mutex_t* mutex = _mutex;
    pthread_mutex_unlock(mutex);
}

void ma_mutex_delete(ma_mutex_t* _mutex) {
    pthread_mutex_t* mutex = _mutex;
    pthread_mutex_destroy(mutex);
    ma_free(mutex);
}

ma_sem_t* ma_sem_create(size_t count) {
    ma_sem_t*           sem;
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;

    sem = ma_malloc(sizeof(*sem));
    if (sem == NULL) {
        return NULL;
    }

    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    pthread_cond_init(&sem->cond, &cattr);
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(&sem->mutex, &mattr);
    sem->count = count;

    return sem;
}

bool ma_sem_wait(ma_sem_t* sem, uint32_t timeout) {
    struct timespec ts;
    int             error = 0;
    uint64_t        nsec  = (uint64_t)timeout * 1000000;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    nsec += ts.tv_nsec;
    if (nsec > (1000000000)) {
        ts.tv_sec += nsec / (1000000000);
        nsec %= (1000000000);
    }
    ts.tv_nsec = nsec;

    pthread_mutex_lock(&sem->mutex);
    while (sem->count == 0) {
        if (timeout != MA_WAIT_FOREVER) {
            error = pthread_cond_timedwait(&sem->cond, &sem->mutex, &ts);
            MA_ASSERT(error != EINVAL);
            if (error) {
                goto timeout;
            }
        } else {
            error = pthread_cond_wait(&sem->cond, &sem->mutex);
            MA_ASSERT(error != EINVAL);
        }
    }

    sem->count--;

timeout:
    pthread_mutex_unlock(&sem->mutex);
    return (error != 0);
}

void ma_sem_signal(ma_sem_t* sem) {
    pthread_mutex_lock(&sem->mutex);
    sem->count++;
    pthread_mutex_unlock(&sem->mutex);
    pthread_cond_signal(&sem->cond);
}

void ma_sem_delete(ma_sem_t* sem) {
    pthread_cond_destroy(&sem->cond);
    pthread_mutex_destroy(&sem->mutex);
    ma_free(sem);
}

ma_tick_t ma_tick_current(void) {
    struct timespec ts;
    ma_tick_t       tick;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    tick = ts.tv_sec;
    tick *= 1000000;
    tick += ts.tv_nsec;
    return tick;
}

ma_tick_t ma_tick_from_us(uint32_t us) {
    return (ma_tick_t)us * 1000;
}

void ma_tick_sleep(ma_tick_t tick) {
    struct timespec ts;
    struct timespec remain;

    ts.tv_sec  = tick / (1000000000);
    ts.tv_nsec = tick % (1000000000);
    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &remain) != 0) {
        ts = remain;
    }
}

ma_event_t* ma_event_create(void) {
    ma_event_t*         event;
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;

    event = (ma_event_t*)ma_malloc(sizeof(*event));
    if (event == NULL) {
        return NULL;
    }

    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    pthread_cond_init(&event->cond, &cattr);
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(&event->mutex, &mattr);
    event->flags = 0;

    return event;
}

bool ma_event_wait(ma_event_t* event, uint32_t mask, uint32_t* value, uint32_t timeout) {
    struct timespec ts;
    int             error = 0;
    uint64_t        nsec  = (uint64_t)timeout * 1000000;

    if (timeout != MA_WAIT_FOREVER) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        nsec += ts.tv_nsec;

        ts.tv_sec += nsec / 1000000000;
        ts.tv_nsec = nsec % 1000000000;
    }

    pthread_mutex_lock(&event->mutex);

    while ((event->flags & mask) == 0) {
        if (timeout != MA_WAIT_FOREVER) {
            error = pthread_cond_timedwait(&event->cond, &event->mutex, &ts);
            MA_ASSERT(error != EINVAL);
            if (error) {
                goto timeout;
            }
        } else {
            error = pthread_cond_wait(&event->cond, &event->mutex);
            MA_ASSERT(error != EINVAL);
        }
    }

timeout:
    if (value) {
        *value = event->flags & mask;
    }
    pthread_mutex_unlock(&event->mutex);
    return (error == 0);
}

void ma_event_set(ma_event_t* event, uint32_t value) {
    pthread_mutex_lock(&event->mutex);
    event->flags |= value;
    pthread_mutex_unlock(&event->mutex);
    pthread_cond_signal(&event->cond);
}

void ma_event_clear(ma_event_t* event, uint32_t value) {
    pthread_mutex_lock(&event->mutex);
    event->flags &= ~value;
    pthread_mutex_unlock(&event->mutex);
    pthread_cond_signal(&event->cond);
}

void ma_event_delete(ma_event_t* event) {
    pthread_cond_destroy(&event->cond);
    pthread_mutex_destroy(&event->mutex);
    ma_free(event);
}

ma_mbox_t* ma_mbox_create(size_t size) {
    ma_mbox_t*          mbox;
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;

    mbox = (ma_mbox_t*)ma_malloc(sizeof(*mbox) + size * sizeof(void*));
    if (mbox == NULL) {
        return NULL;
    }

    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    pthread_cond_init(&mbox->cond, &cattr);
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(&mbox->mutex, &mattr);

    mbox->r     = 0;
    mbox->w     = 0;
    mbox->count = 0;
    mbox->size  = size;

    return mbox;
}

bool ma_mbox_fetch(ma_mbox_t* mbox, const void** msg, uint32_t timeout) {
    struct timespec ts;
    int             error = 0;
    uint64_t        nsec  = (uint64_t)timeout * 1000000;

    if (timeout != MA_WAIT_FOREVER) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        nsec += ts.tv_nsec;

        ts.tv_sec += nsec / 1000000000;
        ts.tv_nsec = nsec % 1000000000;
    }

    pthread_mutex_lock(&mbox->mutex);

    while (mbox->count == 0) {
        if (timeout != MA_WAIT_FOREVER) {
            error = pthread_cond_timedwait(&mbox->cond, &mbox->mutex, &ts);
            MA_ASSERT(error != EINVAL);
            if (error) {
                goto timeout;
            }
        } else {
            error = pthread_cond_wait(&mbox->cond, &mbox->mutex);
            MA_ASSERT(error != EINVAL);
        }
    }

    *msg = mbox->msg[mbox->r++];
    if (mbox->r == mbox->size)
        mbox->r = 0;

    mbox->count--;

timeout:
    pthread_mutex_unlock(&mbox->mutex);
    pthread_cond_signal(&mbox->cond);

    return (error == 0);
}

bool ma_mbox_post(ma_mbox_t* mbox, const void* msg, uint32_t timeout) {
    struct timespec ts;
    int             error = 0;
    uint64_t        nsec  = (uint64_t)timeout * 1000000;

    if (timeout != MA_WAIT_FOREVER) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        nsec += ts.tv_nsec;

        ts.tv_sec += nsec / 1000000000;
        ts.tv_nsec = nsec % 1000000000;
    }

    pthread_mutex_lock(&mbox->mutex);

    while (mbox->count == mbox->size) {
        if (timeout != MA_WAIT_FOREVER) {
            error = pthread_cond_timedwait(&mbox->cond, &mbox->mutex, &ts);
            MA_ASSERT(error != EINVAL);
            if (error) {
                goto timeout;
            }
        } else {
            error = pthread_cond_wait(&mbox->cond, &mbox->mutex);
            MA_ASSERT(error != EINVAL);
        }
    }

    mbox->msg[mbox->w++] = msg;
    if (mbox->w == mbox->size)
        mbox->w = 0;

    mbox->count++;

timeout:
    pthread_mutex_unlock(&mbox->mutex);
    pthread_cond_signal(&mbox->cond);

    return (error == 0);
}

void ma_mbox_delete(ma_mbox_t* mbox) {
    pthread_cond_destroy(&mbox->cond);
    pthread_mutex_destroy(&mbox->mutex);
    ma_free(mbox);
}

static void ma_timer_thread(void* arg) {
    ma_timer_t*     timer = arg;
    sigset_t        sigset;
    siginfo_t       si;
    struct timespec tmo;

    timer->thread_id = (pid_t)syscall(SYS_gettid);

    /* Add SIGALRM */
    sigemptyset(&sigset);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
    sigaddset(&sigset, SIGALRM);

    tmo.tv_sec  = 0;
    tmo.tv_nsec = 500 * 1000000;

    while (!timer->exit) {
        int sig = sigtimedwait(&sigset, &si, &tmo);
        if (sig == SIGALRM) {
            if (timer->fn)
                timer->fn(timer, timer->arg);
        }
    }
}

ma_timer_t* ma_timer_create(uint32_t us,
                            void (*fn)(ma_timer_t*, void* arg),
                            void* arg,
                            bool  oneshot) {
    ma_timer_t*     timer;
    struct sigevent sev;
    sigset_t        sigset;

    /* Block SIGALRM in calling thread */
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    timer = (ma_timer_t*)ma_malloc(sizeof(*timer));
    if (timer == NULL) {
        return NULL;
    }

    timer->exit      = false;
    timer->thread_id = 0;
    timer->fn        = fn;
    timer->arg       = arg;
    timer->us        = us;
    timer->oneshot   = oneshot;

    /* Create timer thread */
    timer->thread = ma_thread_create("ma_timer", MA_TIMER_PRIO, 1024, ma_timer_thread, timer);
    if (timer->thread == NULL) {
        ma_free(timer);
        return NULL;
    }

    /* Wait until timer thread sets its (kernel) thread id */
    do {
        sched_yield();
    } while (timer->thread_id == 0);


    /* Create timer */
    sev.sigev_notify          = SIGEV_THREAD_ID;
    sev.sigev_value.sival_ptr = timer;
#ifdef __x86_64__
    sev._sigev_un._tid = timer->thread_id;
#else
    sev.sigev_notify_thread_id = timer->thread_id;
#endif
    sev.sigev_signo             = SIGALRM;
    sev.sigev_notify_attributes = NULL;

    if (timer_create(CLOCK_MONOTONIC, &sev, &timer->timerid) == -1) {
        ma_free(timer);
        return NULL;
    }

    return timer;
}

void ma_timer_set(ma_timer_t* timer, uint32_t us) {
    timer->us = us;
}

void ma_timer_start(ma_timer_t* timer) {
    struct itimerspec its;

    /* Start timer */
    its.it_value.tv_sec     = 0;
    its.it_value.tv_nsec    = 1000 * timer->us;
    its.it_interval.tv_sec  = (timer->oneshot) ? 0 : its.it_value.tv_sec;
    its.it_interval.tv_nsec = (timer->oneshot) ? 0 : its.it_value.tv_nsec;
    timer_settime(timer->timerid, 0, &its, NULL);
}

void ma_timer_stop(ma_timer_t* timer) {
    struct itimerspec its;

    /* Stop timer */
    its.it_value.tv_sec     = 0;
    its.it_value.tv_nsec    = 0;
    its.it_interval.tv_sec  = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(timer->timerid, 0, &its, NULL);
}

void ma_timer_delete(ma_timer_t* timer) {
    timer->exit = true;
    pthread_join(*timer->thread, NULL);
    timer_delete(timer->timerid);
    ma_free(timer);
}

#endif