#include "ma_misc.h"

#include <stdint.h>
#include <time.h>

#include "osal/ma_osal_freertos.h"

MA_ATTR_WEAK void ma_usleep(uint32_t usec) { vTaskDelay(usec / 1000 / portTICK_PERIOD_MS); }

MA_ATTR_WEAK void ma_sleep(uint32_t msec) { ma_usleep(msec * 1000); }

MA_ATTR_WEAK int64_t ma_get_time_us(void) { return xTaskGetTickCount() * portTICK_PERIOD_MS * 1000; }

MA_ATTR_WEAK int64_t ma_get_time_ms(void) { return xTaskGetTickCount() * portTICK_PERIOD_MS; }