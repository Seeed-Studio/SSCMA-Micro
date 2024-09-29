#ifndef _MA_BOARD_WATCHER_H_
#define _MA_BOARD_WATCHER_H_

#define MA_BOARD_NAME "Watcher"

#define MA_CAMERA_PWR_CTRL_INIT_F                               \
    {                                                           \
        hx_drv_gpio_set_output(AON_GPIO1, GPIO_OUT_LOW);        \
        hx_drv_scu_set_PA1_pinmux(SCU_PA1_PINMUX_AON_GPIO1, 0); \
        hx_drv_gpio_set_out_value(AON_GPIO1, GPIO_OUT_LOW);     \
    }

#endif