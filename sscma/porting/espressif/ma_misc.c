
#include "core/ma_common.h"

#include <stdint.h>
#include <time.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

MA_ATTR_WEAK void ma_usleep(uint32_t usec) {
    vTaskDelay(usec / 1000 / portTICK_PERIOD_MS);
}

MA_ATTR_WEAK void ma_sleep(uint32_t msec) {
    ma_usleep(msec * 1000);
}

MA_ATTR_WEAK int64_t ma_get_time_us(void) {
    return (int64_t)esp_timer_get_time();
}

MA_ATTR_WEAK int64_t ma_get_time_ms(void) {
    return (int64_t)esp_timer_get_time() / 1000;
}
