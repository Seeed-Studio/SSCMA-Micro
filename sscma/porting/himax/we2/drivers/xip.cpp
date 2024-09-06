#include "xip.h"

#include <cstddef>

extern "C" {
#include <spi_eeprom_comm.h>
}

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <core/ma_debug.h>
#include <porting/ma_osal.h>

static const char*   TAG = "device::xip";
static ma::Mutex     __owner_mutex;
static volatile bool __xip_initialized = false;
static volatile bool __xip_enabled     = false;

void xip_drv_init() {
    xip_ownership_acquire();
    if (__xip_initialized) {
        xip_ownership_release();
        return;
    }
    __xip_initialized = true;
    int ret           = hx_lib_spi_eeprom_open(USE_DW_SPI_MST_Q);
    if (ret != E_OK) {
        MA_LOGE(TAG, "Failed to open SPI EEPROM");
        ma_abort();
    }
    xip_ownership_release();
}

void xip_ownership_acquire() { __owner_mutex.lock(); }

void xip_ownership_release() { __owner_mutex.unlock(); }

bool xip_safe_enable() {
    if (!__xip_initialized) {
        return false;
    }
    if (!__xip_enabled) {
        return __xip_enabled = (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true) == E_OK);
    }
    return true;
}

bool xip_safe_disable() {
    if (!__xip_initialized) {
        return false;
    }
    if (__xip_enabled) {
        bool disabled = (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, false, FLASH_QUAD, false) == E_OK);
        __xip_enabled = !disabled;
        return disabled;
    }
    return true;
}
