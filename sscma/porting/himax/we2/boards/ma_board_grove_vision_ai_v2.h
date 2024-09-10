#ifndef _MA_BOARD_GROVE_VISION_AI_V2_H_
#define _MA_BOARD_GROVE_VISION_AI_V2_H_

#define MA_BOARD_NAME "Grove Vision AI V2"

#define MA_CAMERA_PWR_CTRL_INIT_F                               \
    {                                                           \
        hx_drv_gpio_set_output(AON_GPIO1, GPIO_OUT_HIGH);       \
        hx_drv_scu_set_PA1_pinmux(SCU_PA1_PINMUX_AON_GPIO1, 0); \
        hx_drv_gpio_set_out_value(AON_GPIO1, GPIO_OUT_HIGH);    \
    }

#endif