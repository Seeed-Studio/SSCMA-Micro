
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "ma_config_board.h"
#include <porting/ma_misc.h>

void ma_usleep(uint32_t usec) {
    usleep(usec);
}

void ma_sleep(uint32_t msec) {
    ma_usleep(msec * 1000);
}

int64_t ma_get_time_us(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);

    int64_t t_us = t.tv_sec * 1000000 + t.tv_nsec / 1000;
    return t_us;
}

int64_t ma_get_time_ms(void) {
    return ma_get_time_us() / 1000;
}