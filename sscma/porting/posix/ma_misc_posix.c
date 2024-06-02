#include "core/ma_common.h"

#if MA_PORTING_POSIX == 1

#include <stdint.h>
#include <time.h>

#include "porting/posix/ma_misc_posix.h"


MA_ATTR_WEAK void ma_usleep(uint32_t usec) {
    struct timespec ts;
    struct timespec remain;

    ts.tv_sec  = usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;
    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &remain) != 0) {
        ts = remain;
    }
}

MA_ATTR_WEAK void ma_sleep(uint32_t msec) {
    ma_usleep(msec * 1000);
}

MA_ATTR_WEAK int64_t ma_get_time_us(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 * 1000 + ts.tv_nsec / 1000;
}

MA_ATTR_WEAK int64_t ma_get_time_ms(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

#endif