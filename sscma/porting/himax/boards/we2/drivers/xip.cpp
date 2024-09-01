#include "xip.h"

#include <cstddef>

extern "C" {
#include <spi_eeprom_comm.h>
}

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "porting/ma_osal.h"

#define XIP_STA_CHANGE_RETRY 10

static volatile bool __xip_enabled = false;
static ma::Mutex     __xip_mutex;

bool xip_safe_enable() {
    ma::Guard guard(__xip_mutex);
    if (!__xip_enabled) {
        int retry = XIP_STA_CHANGE_RETRY;
        while (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true) != E_OK) {
            puts("xip_safe_enable: failed to enable XIP mode");
            if (--retry < 0) {
                return false;
            }
            ma::Thread::yield();
        }
        __xip_enabled = true;
    }
    return true;
}

bool xip_safe_disable() {
    ma::Guard guard(__xip_mutex);
    if (__xip_enabled) {
        int retry = XIP_STA_CHANGE_RETRY;
        while (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, false, FLASH_QUAD, false) != E_OK) {
            puts("xip_safe_disable: failed to disable XIP mode");
            if (--retry < 0) {
                return false;
            }
            ma::Thread::yield();
        }
        __xip_enabled = false;
    }
    return true;
}
