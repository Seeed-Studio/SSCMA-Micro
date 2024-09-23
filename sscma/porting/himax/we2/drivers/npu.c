#include "npu.h"

#include <WE2_ARMCM55.h>
#include <WE2_core.h>
#include <ethosu_driver.h>
#include <hx_drv_pmu.h>
#include <hx_drv_watchdog.h>
#include <spi_eeprom_comm.h>
#include <stdio.h>

#include "core/ma_config_internal.h"
#include "core/ma_debug.h"

struct ethosu_driver _ethosu_drv; /* Default Ethos-U device driver */

static void _arm_npu_irq_handler(void) {
    /* Call the default interrupt handler from the NPU driver */
    ethosu_irq_handler(&_ethosu_drv);
}

static void _arm_npu_irq_init(void) {
    const IRQn_Type ethosu_irqnum = (IRQn_Type)U55_IRQn;

    /* Register the EthosU IRQ handler in our vector table.
     * Note, this handler comes from the EthosU driver */
    EPII_NVIC_SetVector(ethosu_irqnum, (uint32_t)_arm_npu_irq_handler);

    /* Enable the IRQ */
    NVIC_EnableIRQ(ethosu_irqnum);
}

int himax_arm_npu_init(bool security_enable, bool privilege_enable) {
    int err = 0;

    /* Initialise the IRQ */
    _arm_npu_irq_init();

    /* Initialise Ethos-U55 device */
    void* const ethosu_base_address = (void*)(BASE_ADDR_APB_U55_CTRL_ALIAS);

    if (0 != (err = ethosu_init(&_ethosu_drv,         /* Ethos-U driver device pointer */
                                ethosu_base_address,  /* Ethos-U NPU's base address. */
                                NULL,                 /* Pointer to fast mem area - NULL for U55. */
                                0,                    /* Fast mem region size. */
                                security_enable,      /* Security enable. */
                                privilege_enable))) { /* Privilege enable. */
        MA_LOGD(MA_TAG, "Ethos-U55 device initialisation failed: %d", err);
        return err;
    }

    MA_LOGD(MA_TAG, "Ethos-U55 device initialised");

    return 0;
}