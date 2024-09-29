#include "drv_imx219.h"

#include <WE2_core.h>
#include <WE2_device_addr.h>
#include <core/ma_debug.h>
#include <ctype.h>
#include <drv_shared_cfg.h>
#include <hx_drv_CIS_common.h>
#include <hx_drv_hxautoi2c_mst.h>
#include <hx_drv_inp.h>
#include <hx_drv_scu.h>
#include <hx_drv_scu_export.h>
#include <hx_drv_swreg_aon.h>
#include <hx_drv_timer.h>
#include <ma_config_board.h>
#include <porting/ma_misc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drv_shared_cfg.h"
#include "drv_shared_state.h"

#define IMX219_SENSOR_I2CID       0x10
#define IMX219_MIPI_CLOCK_FEQ     456  // MHz
#define IMX219_MIPI_LANE_CNT      2
#define IMX219_MIPI_DPP           10  // depth per pixel
#define IMX219_MIPITX_CNTCLK_EN   1   // continuous clock output enable
#define IMX219_LANE_NB            2

#define SENSORDPLIB_SENSOR_IMX219 (SENSORDPLIB_SENSOR_HM2130)

#define IMX219_SENSOR_WIDTH       3280
#define IMX219_SENSOR_HEIGHT      2464

#define IMX219_EXPOSURE_DEFAULT   0x640
#define IMX219_EXPOSURE_SETTING   0xA40
#define IMX219_ANA_GAIN_MIN       0
#define IMX219_ANA_GAIN_MAX       232
#define IMX219_ANA_GAIN_DEFAULT   0x0
#define IMX219_AGAIN_SETTING      0x40
#define IMX219_DGTL_GAIN_MIN      0x0100
#define IMX219_DGTL_GAIN_MAX      0x0fff
#define IMX219_DGTL_GAIN_DEFAULT  0x0100
#define IMX219_DGAIN_SETTING      0x200

#define IMX219_REG_BINNING_MODE   0x0174
#define IMX219_BINNING_NONE       0x0000
#define IMX219_BINNING_2X2        0x0101
#define IMX219_BINNING_2X2_ANALOG 0x0303
#define IMX219_BINNING_SETTING    IMX219_BINNING_NONE

#define CIS_MIRROR_SETTING        0x03  // 0x00: off / 0x01: H-Mirror / 0x02:V-Mirror / 0x03: HV-Mirror
#define CIS_I2C_ID                IMX219_SENSOR_I2CID
#define CIS_ENABLE_MIPI_INF       0x01  // 0x00: off / 0x01: on
#define CIS_MIPI_LANE_NUMBER      0x02
#define CIS_ENABLE_HX_AUTOI2C     0x00  // 0x00: off / 0x01: on / 0x2: on and XSLEEP KEEP HIGH
#define DEAULT_XHSUTDOWN_PIN      AON_GPIO2

#define SENCTRL_SENSOR_TYPE       SENSORDPLIB_SENSOR_IMX219
#define SENCTRL_STREAM_TYPE       SENSORDPLIB_STREAM_NONEAOS
#define SENCTRL_SENSOR_WIDTH      IMX219_SENSOR_WIDTH
#define SENCTRL_SENSOR_HEIGHT     IMX219_SENSOR_HEIGHT
#define SENCTRL_SENSOR_CH         3

static bool _is_version_c = false;

static HX_CIS_SensorSetting_t IMX219_init_setting[] = {
#include "IMX219_mipi_2lane_3280x2464.i"
};

static HX_CIS_SensorSetting_t IMX219_stream_on[] = {
  {HX_CIS_I2C_Action_W, 0x0100, 0x01},
};

static HX_CIS_SensorSetting_t IMX219_stream_off[] = {
  {HX_CIS_I2C_Action_W, 0x0100, 0x00},
};

static HX_CIS_SensorSetting_t IMX219_binning_setting[] = {
  {HX_CIS_I2C_Action_W, 0x0174, ((IMX219_BINNING_SETTING >> 8) & 0xFF)},
  {HX_CIS_I2C_Action_W, 0x0175,        (IMX219_BINNING_SETTING & 0xFF)},
};

static HX_CIS_SensorSetting_t IMX219_exposure_setting[] = {
  {HX_CIS_I2C_Action_W, 0x015a, ((IMX219_EXPOSURE_SETTING >> 8) & 0xFF)},
  {HX_CIS_I2C_Action_W, 0x015b,        (IMX219_EXPOSURE_SETTING & 0xFF)},
};

static HX_CIS_SensorSetting_t IMX219_again_setting[] = {
  {HX_CIS_I2C_Action_W, 0x0157, (IMX219_AGAIN_SETTING & 0xFF)},
};

static HX_CIS_SensorSetting_t IMX219_dgain_setting[] = {
  {HX_CIS_I2C_Action_W, 0x0158, ((IMX219_DGAIN_SETTING >> 8) & 0xFF)},
  {HX_CIS_I2C_Action_W, 0x0159,        (IMX219_DGAIN_SETTING & 0xFF)},
};

static HX_CIS_SensorSetting_t IMX219_mirror_setting[] = {
  {HX_CIS_I2C_Action_W, 0x0172, (CIS_MIRROR_SETTING & 0xFF)},
};

ma_err_t drv_imx219_probe() {
    if (hx_drv_cis_init((CIS_XHSHUTDOWN_INDEX_E)DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV3) != HX_CIS_NO_ERROR) {
        MA_LOGD(MA_TAG, "Camera CIS Init Fail");
        return MA_FAILED;
    }

    MA_CAMERA_PWR_CTRL_INIT_F;
    hx_drv_timer_cm55x_delay_ms(10, TIMER_STATE_DC);

    if (hx_drv_cis_set_slaveID(CIS_I2C_ID) != HX_CIS_NO_ERROR) {
        return MA_FAILED;
    }

    uint8_t data;
    if (hx_drv_cis_get_reg(0x0000, &data) != HX_CIS_NO_ERROR) {
        return MA_FAILED;
    }
    MA_LOGD(MA_TAG, "IMX219 ID: %02X", data);

    return MA_OK;
}

static uint32_t imx219_set_pll200() {
    SCU_PDHSC_DPCLK_CFG_T cfg;

    if (hx_drv_scu_get_pdhsc_dpclk_cfg(&cfg) != SCU_NO_ERROR) {
        MA_LOGE(MA_TAG, "Get SCU PDHSC DPCLK CFG failed");
    }

    uint32_t pllfreq;
    hx_drv_swreg_aon_get_pllfreq(&pllfreq);

    if (pllfreq == 400000000) {
        cfg.mipiclk.hscmipiclksrc = SCU_HSCMIPICLKSRC_PLL;
        cfg.mipiclk.hscmipiclkdiv = 1;
    } else {
        cfg.mipiclk.hscmipiclksrc = SCU_HSCMIPICLKSRC_PLL;
        cfg.mipiclk.hscmipiclkdiv = 0;
    }

    if (hx_drv_scu_set_pdhsc_dpclk_cfg(cfg, 0, 1) != SCU_NO_ERROR) {
        MA_LOGE(MA_TAG, "Set SCU PDHSC DPCLK CFG failed");
    }

    uint32_t mipi_pixel_clk = 96;
    if (hx_drv_scu_get_freq(SCU_CLK_FREQ_TYPE_HSC_MIPI_RXCLK, &mipi_pixel_clk) != SCU_NO_ERROR) {
        MA_LOGE(MA_TAG, "Get SCU HSC MIPI RXCLK failed");
    }
    mipi_pixel_clk = mipi_pixel_clk / 1000000;

    return mipi_pixel_clk;
}

static void set_mipi_csirx_enable() {
    uint32_t bitrate_1lane  = IMX219_MIPI_CLOCK_FEQ * 2;
    uint32_t mipi_lnno      = IMX219_MIPI_LANE_CNT;
    uint32_t pixel_dpp      = IMX219_MIPI_DPP;
    uint32_t line_length    = IMX219_SENSOR_WIDTH;
    uint32_t frame_length   = IMX219_SENSOR_HEIGHT;
    uint32_t byte_clk       = bitrate_1lane / 8;
    uint32_t continuousout  = IMX219_MIPITX_CNTCLK_EN;
    uint32_t deskew_en      = 0;
    uint32_t mipi_pixel_clk = imx219_set_pll200();

    MA_LOGD(MA_TAG, "MIPI Bitrate: %d", bitrate_1lane);
    MA_LOGD(MA_TAG, "MIPI Lane: %d", mipi_lnno);
    MA_LOGD(MA_TAG, "MIPI Pixel Depth: %d", pixel_dpp);
    MA_LOGD(MA_TAG, "MIPI Line Length: %d", line_length);
    MA_LOGD(MA_TAG, "MIPI Frame Length: %d", frame_length);
    MA_LOGD(MA_TAG, "MIPI Byte Clock: %d", byte_clk);
    MA_LOGD(MA_TAG, "MIPI Continuous Clock: %d", continuousout);
    MA_LOGD(MA_TAG, "MIPI Pixel Clock: %d", mipi_pixel_clk);

    uint32_t n_preload = 15;
    uint32_t l_header  = 4;
    uint32_t l_footer  = 2;

    double t_input   = (double)(l_header + line_length * pixel_dpp / 8 + l_footer) / (mipi_lnno * byte_clk) + 0.06;
    double t_output  = (double)line_length / mipi_pixel_clk;
    double t_preload = (double)(7 + (n_preload * 4) / mipi_lnno) / mipi_pixel_clk;
    double delta_t   = t_input - t_output - t_preload;

    MA_LOGD(MA_TAG, "MIPI Input: %dns", (uint32_t)(t_input * 1000));
    MA_LOGD(MA_TAG, "MIPI Output: %dns", (uint32_t)(t_output * 1000));
    MA_LOGD(MA_TAG, "MIPI Preload: %dns", (uint32_t)(t_preload * 1000));

    uint16_t rx_fifo_fill = 0;
    uint16_t tx_fifo_fill = 0;

    if (delta_t <= 0) {
        delta_t      = 0 - delta_t;
        tx_fifo_fill = ceil(delta_t * byte_clk * mipi_lnno / 4 / (pixel_dpp / 2)) * (pixel_dpp / 2);
        rx_fifo_fill = 0;
    } else {
        rx_fifo_fill = ceil(delta_t * byte_clk * mipi_lnno / 4 / (pixel_dpp / 2)) * (pixel_dpp / 2);
        tx_fifo_fill = 0;
    }
    MA_LOGD(MA_TAG, "MIPI RX FIFO Fill: %d", rx_fifo_fill);
    MA_LOGD(MA_TAG, "MIPI TX FIFO Fill: %d", tx_fifo_fill);

    MA_LOGD(MA_TAG, "MIPI Reset");

    SCU_DP_SWRESET_T dp_swrst;
    if (drv_interface_get_dp_swreset(&dp_swrst) != 0) {
        MA_LOGE(MA_TAG, "Get DP SW Reset failed");
    }
    dp_swrst.HSC_MIPIRX = 0;
    dp_swrst.HSC_MIPITX = 0;

    if (hx_drv_scu_set_DP_SWReset(dp_swrst) != SCU_NO_ERROR) {
        MA_LOGE(MA_TAG, "Set DP SW Reset failed");
    }
    hx_drv_timer_cm55x_delay_us(50, TIMER_STATE_DC);

    dp_swrst.HSC_MIPIRX = 1;
    dp_swrst.HSC_MIPITX = 1;
    if (hx_drv_scu_set_DP_SWReset(dp_swrst) != SCU_NO_ERROR) {
        MA_LOGE(MA_TAG, "Set DP SW Reset failed");
    }

    MIPIRX_DPHYHSCNT_CFG_T hscnt_cfg;
    hscnt_cfg.mipirx_dphy_hscnt_clk_en = 0;
    hscnt_cfg.mipirx_dphy_hscnt_ln0_en = 1;
    hscnt_cfg.mipirx_dphy_hscnt_ln1_en = 1;

    hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
    hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x10;
    hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x10;

    sensordplib_csirx_set_hscnt(hscnt_cfg);

    if (pixel_dpp == 10 || pixel_dpp == 8) {
        sensordplib_csirx_set_pixel_depth(pixel_dpp);
    } else {
        MA_LOGE(MA_TAG, "Invalid pixel depth %d", pixel_dpp);
        return;
    }

    sensordplib_csirx_set_deskew(deskew_en);
    sensordplib_csirx_set_fifo_fill(rx_fifo_fill);
    sensordplib_csirx_enable(mipi_lnno);

    CSITX_DPHYCLKMODE_E clkmode;
    if (continuousout) {
        clkmode = CSITX_DPHYCLOCK_CONT;
    } else {
        clkmode = CSITX_DPHYCLOCK_NON_CONT;
    }

    sensordplib_csitx_set_dphy_clkmode(clkmode);

    sensordplib_csitx_set_deskew(deskew_en);
    sensordplib_csitx_set_fifo_fill(tx_fifo_fill);
    sensordplib_csitx_enable(mipi_lnno, bitrate_1lane, line_length, frame_length);

    SCU_VMUTE_CFG_T ctrl;
    ctrl.timingsrc = SCU_VMUTE_CTRL_TIMING_SRC_VMUTE;
    ctrl.txphypwr  = SCU_VMUTE_CTRL_TXPHY_PWR_DISABLE;
    ctrl.ctrlsrc   = SCU_VMUTE_CTRL_SRC_SW;
    ctrl.swctrl    = SCU_VMUTE_CTRL_SW_ENABLE;
    if (hx_drv_scu_set_vmute(&ctrl) != SCU_NO_ERROR) {
        MA_LOGE(MA_TAG, "Set SCU VMUTE failed");
    }

    MA_LOGD(MA_TAG, "MIPI VMUTE: 0x%08X", *(uint32_t*)(SCU_LSC_ADDR + 0x408));
    MA_LOGD(MA_TAG, "0x53061000: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x1000));
    MA_LOGD(MA_TAG, "0x53061004: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x1004));
    MA_LOGD(MA_TAG, "0x53061008: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x1008));
    MA_LOGD(MA_TAG, "0x5306100C: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x100C));
    MA_LOGD(MA_TAG, "0x53061010: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x1010));
}

static void set_mipi_csirx_disable() {
    EL_LOGD("MIPI CSI Disable");

    volatile uint32_t* dphy_reg                  = CSIRX_DPHY_REG;
    volatile uint32_t* csi_static_cfg_reg        = (CSIRX_REGS_BASE + 0x08);
    volatile uint32_t* csi_dphy_lane_control_reg = (CSIRX_REGS_BASE + 0x40);
    volatile uint32_t* csi_stream0_control_reg   = (CSIRX_REGS_BASE + 0x100);
    volatile uint32_t* csi_stream0_data_cfg      = (CSIRX_REGS_BASE + 0x108);
    volatile uint32_t* csi_stream0_cfg_reg       = (CSIRX_REGS_BASE + 0x10C);

    sensordplib_csirx_disable();

    EL_LOGD("0x%08X = 0x%08X", dphy_reg, *dphy_reg);
    EL_LOGD("0x%08X = 0x%08X", csi_static_cfg_reg, *csi_static_cfg_reg);
    EL_LOGD("0x%08X = 0x%08X", csi_dphy_lane_control_reg, *csi_dphy_lane_control_reg);
    EL_LOGD("0x%08X = 0x%08X", csi_stream0_data_cfg, *csi_stream0_data_cfg);
    EL_LOGD("0x%08X = 0x%08X", csi_stream0_control_reg, *csi_stream0_control_reg);
}

static void _on_frame_ready_cb() {
    int ec = hx_drv_cis_setRegTable(IMX219_stream_off, HX_CIS_SIZE_N(IMX219_stream_off, HX_CIS_SensorSetting_t));
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "On frame ready fail: %d", ec);
    }
    set_mipi_csirx_disable();
}

static void _on_next_capture_cb() {
    set_mipi_csirx_enable();

    int ec = hx_drv_cis_setRegTable(IMX219_stream_on, HX_CIS_SIZE_N(IMX219_stream_on, HX_CIS_SensorSetting_t));
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "On stop capture fail: %d", ec);
    }
}

ma_err_t drv_imx219_init(uint16_t width, uint16_t height, int compression) {
    if (!width || !height || width > FRAME_WIDTH_MAX || height > FRAME_HEIGHT_MAX) {
        MA_LOGE(MA_TAG, "Invalid resolution %dx%d", width, height);
        return MA_EINVAL;
    }

    uint32_t chipid;
    uint32_t version;
    hx_drv_scu_get_version(&chipid, &version);
    if (chipid == 0x8538000c) {
        _is_version_c                   = true;
        _initiated_before               = false;
        _drv_dp_event_cb_on_frame_ready = _on_frame_ready_cb;
        _drv_dp_on_next_stream          = _on_next_capture_cb;
    } else {
        _is_version_c                   = false;
        _drv_dp_event_cb_on_frame_ready = NULL;
        _drv_dp_on_next_stream          = NULL;
    }

    ma_err_t    ret = MA_OK;
    int         ec;
    HW5x5_CFG_T hw5x5_cfg;
    JPEG_CFG_T  jpeg_cfg;
    uint16_t    start_x;
    uint16_t    start_y;
    uint16_t    w;
    uint16_t    h;
    INP_CROP_T  crop;

#if ENABLE_SENSOR_FAST_SWITCH
    MA_LOGD(MA_TAG, "Camera Initiated Before: %d", _initiated_before);
    if (!_initiated_before) {
#endif
        MA_LOGD(MA_TAG, "Camera Init Pin: %d", DEAULT_XHSUTDOWN_PIN);
        ec = hx_drv_cis_init((CIS_XHSHUTDOWN_INDEX_E)DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV3);
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera CIS Init Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        MA_CAMERA_PWR_CTRL_INIT_F;
        hx_drv_timer_cm55x_delay_ms(10, TIMER_STATE_DC);

        ec = hx_drv_cis_set_slaveID(CIS_I2C_ID);
        MA_LOGD(MA_TAG, "Camera Set Slave ID %d: %d", CIS_I2C_ID, ec);
        if (ec != HX_CIS_NO_ERROR) {
            ret = MA_EIO;
            goto err;
        }

        hx_drv_timer_cm55x_delay_ms(3, TIMER_STATE_DC);

        ec = hx_drv_cis_setRegTable(IMX219_stream_off, HX_CIS_SIZE_N(IMX219_stream_off, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Stream Off Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        ec = hx_drv_cis_setRegTable(IMX219_init_setting, HX_CIS_SIZE_N(IMX219_init_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Init Setting Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        ec =
          hx_drv_cis_setRegTable(IMX219_binning_setting, HX_CIS_SIZE_N(IMX219_binning_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Binning Setting Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        ec = hx_drv_cis_setRegTable(IMX219_exposure_setting,
                                    HX_CIS_SIZE_N(IMX219_exposure_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Exposure Setting Fail: %d", ec);
        }

        ec = hx_drv_cis_setRegTable(IMX219_again_setting, HX_CIS_SIZE_N(IMX219_again_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Again Setting Fail: %d", ec);
        }

        ec = hx_drv_cis_setRegTable(IMX219_dgain_setting, HX_CIS_SIZE_N(IMX219_dgain_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Dgain Setting Fail: %d", ec);
        }

        ec =
          hx_drv_cis_setRegTable(IMX219_mirror_setting, HX_CIS_SIZE_N(IMX219_mirror_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Mirror Setting Fail: %d", ec);
        }

        set_mipi_csirx_enable();

#if ENABLE_SENSOR_FAST_SWITCH
    }
#endif

    crop.start_x = 0;
    crop.start_y = 0;
    crop.last_x  = 0;
    crop.last_y  = 0;

    int32_t factor_w = floor((float)IMX219_SENSOR_WIDTH / (float)width);
    int32_t factor_h = floor((float)IMX219_SENSOR_HEIGHT / (float)height);
    int32_t min_f    = factor_w < factor_h ? factor_w : factor_h;

    if (min_f >= 16) {
        w  = IMX219_SENSOR_WIDTH / 16;
        h  = IMX219_SENSOR_HEIGHT / 16;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX219_SENSOR_WIDTH,
                                                        IMX219_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_4TO2,
                                                        crop,
                                                        INP_BINNING_16TO2_B);
    } else if (min_f >= 8) {
        w  = IMX219_SENSOR_WIDTH / 8;
        h  = IMX219_SENSOR_HEIGHT / 8;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX219_SENSOR_WIDTH,
                                                        IMX219_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_DISABLE,
                                                        crop,
                                                        INP_BINNING_16TO2_B);
    } else if (min_f >= 5) {
        w  = IMX219_SENSOR_WIDTH / 5;
        h  = IMX219_SENSOR_HEIGHT / 5;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX219_SENSOR_WIDTH,
                                                        IMX219_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_DISABLE,
                                                        crop,
                                                        INP_BINNING_10TO2_B);
    } else if (min_f >= 4) {
        w  = IMX219_SENSOR_WIDTH / 4;
        h  = IMX219_SENSOR_HEIGHT / 4;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX219_SENSOR_WIDTH,
                                                        IMX219_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_DISABLE,
                                                        crop,
                                                        INP_BINNING_8TO2_B);
    } else {
        w  = IMX219_SENSOR_WIDTH;
        h  = IMX219_SENSOR_HEIGHT;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX219_SENSOR_WIDTH,
                                                        IMX219_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_DISABLE,
                                                        crop,
                                                        INP_SUBSAMPLE_DISABLE);
    }

    start_x = (w - width) / 2;
    start_y = (h - height) / 2;

    MA_LOGD(MA_TAG, "Start X: %d Start Y: %d Width: %d Height: %d", start_x, start_y, width, height);

    _reset_all_wdma_buffer();

    _frame.width  = width;
    _frame.height = height;
    _frame.rotate = MA_PIXEL_ROTATE_0;
    _frame.format = MA_PIXEL_FORMAT_YUV422;
    _frame.size   = width * height * 3 / 2;
    _frame.data   = _wdma3_baseaddr;
    _jpeg.width   = width;
    _jpeg.height  = height;
    _jpeg.rotate  = MA_PIXEL_ROTATE_0;
    _jpeg.format  = MA_PIXEL_FORMAT_JPEG;
    _jpeg.size    = width * height / 4;
    _jpeg.data    = _wdma2_baseaddr;

    MA_LOGD(MA_TAG, "Frame Size: %d JPEG Size: %d", _frame.size, _jpeg.size);
    MA_LOGD(MA_TAG, "Frame Data: %x JPEG Data: %x", _frame.data, _jpeg.data);

#if ENABLE_SENSOR_FAST_SWITCH
    if (!_initiated_before) {
#endif
        sensordplib_set_xDMA_baseaddrbyapp(_wdma1_baseaddr, _wdma2_baseaddr, _wdma3_baseaddr);
        sensordplib_set_jpegfilesize_addrbyapp(_jpegsize_baseaddr);
#if ENABLE_SENSOR_FAST_SWITCH
    }
#endif

    hw5x5_cfg.hw5x5_path         = HW5x5_PATH_THROUGH_DEMOSAIC;
    hw5x5_cfg.demos_bndmode      = DEMOS_BNDODE_REFLECT;
    hw5x5_cfg.demos_color_mode   = DEMOS_COLORMODE_YUV422;
    hw5x5_cfg.demos_pattern_mode = DEMOS_PATTENMODE_GBRG;
    hw5x5_cfg.demoslpf_roundmode = DEMOSLPF_ROUNDMODE_ROUNDING;
    hw5x5_cfg.hw55_crop_stx      = start_x;
    hw5x5_cfg.hw55_crop_sty      = start_y;
    hw5x5_cfg.hw55_in_width      = width;
    hw5x5_cfg.hw55_in_height     = height;

    jpeg_cfg.jpeg_path      = JPEG_PATH_ENCODER_EN;
    jpeg_cfg.enc_width      = width;
    jpeg_cfg.enc_height     = height;
    jpeg_cfg.jpeg_enctype   = JPEG_ENC_TYPE_YUV422;
    jpeg_cfg.jpeg_encqtable = JPEG_ENC_QTABLE_10X;

    switch (compression) {
    case 10:
        jpeg_cfg.jpeg_encqtable = JPEG_ENC_QTABLE_10X;
        break;
    case 4:
        jpeg_cfg.jpeg_encqtable = JPEG_ENC_QTABLE_4X;
        break;
    default:
        MA_LOGE(MA_TAG, "Invalid compression %d", compression);
        ret = MA_EINVAL;
        goto err;
    }

    sensordplib_set_int_hw5x5_jpeg_wdma23(hw5x5_cfg, jpeg_cfg, 1, NULL);

#if ENABLE_SENSOR_FAST_SWITCH
    MA_LOGD(MA_TAG, "Camera Initiated Before: %d", _initiated_before);
    if (!_initiated_before) {
#endif

        hx_dplib_register_cb(_drv_dp_event_cb, SENSORDPLIB_CB_FUNTYPE_DP);

        ec = hx_drv_cis_setRegTable(IMX219_stream_on, HX_CIS_SIZE_N(IMX219_stream_on, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Stream On Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        sensordplib_set_mclkctrl_xsleepctrl_bySCMode();

#if ENABLE_SENSOR_FAST_SWITCH
    }
#endif

    if (_is_version_c && _on_next_capture_cb) {
        _on_next_capture_cb();
    }

    sensordplib_set_sensorctrl_start();

    _frame_ready = false;
    ++_frame_count;
    sensordplib_retrigger_capture();

    if (_is_version_c) {
        int retry = 0;
        while (!_frame_ready) {
            hx_drv_timer_cm55x_delay_ms(10, TIMER_STATE_DC);
            if (++retry > 100) {
                MA_LOGE(MA_TAG, "Wait frame ready timeout");
                ret = MA_ETIMEOUT;
                goto err;
            }
        }
        if (_on_next_capture_cb) {
            _on_next_capture_cb();
        }
    }

    _initiated_before = true;

    MA_LOGD(MA_TAG, "Camera Init Success!");

    return ret;

err:
    MA_LOGD(MA_TAG, "Camera Init Failed!");

#if ENABLE_SENSOR_FAST_SWITCH
    _initiated_before = false;

    sensordplib_stop_capture();
    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();

    ec = hx_drv_cis_setRegTable(IMX219_stream_off, HX_CIS_SIZE_N(IMX219_stream_off, HX_CIS_SensorSetting_t));
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "Camera Set Stream Off Fail: %d", ec);
    }

    set_mipi_csirx_disable();
#endif

    hx_drv_sensorctrl_set_xSleep(1);

    return ret;
}

void drv_imx219_deinit() {
    MA_LOGD(MA_TAG, "Camera Deinit");

    int ec;

    sensordplib_stop_capture();

#if !ENABLE_SENSOR_FAST_SWITCH
    _frame_count = 0;

    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();

    ec = hx_drv_cis_setRegTable(IMX219_stream_off, HX_CIS_SIZE_N(IMX219_stream_off, HX_CIS_SensorSetting_t));
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "Camera Set Stream Off Fail: %d", ec);
    }
    set_mipi_csirx_disable();

#endif

    hx_drv_sensorctrl_set_xSleep(1);

    hx_drv_timer_cm55x_delay_ms(3, TIMER_STATE_DC);
}
