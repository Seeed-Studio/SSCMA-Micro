#include "drv_imx708.h"

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

#define IMX708_SENSOR_WIDTH           2304
#define IMX708_SENSOR_HEIGHT          1296

#define IMX708_QBC_ADJUST             0x02
#define IMX708_REG_BASE_SPC_GAINS_L   0x7b10
#define IMX708_REG_BASE_SPC_GAINS_R   0x7c00
#define IMX708_LPF_INTENSITY_EN       0xC428
#define IMX708_LPF_INTENSITY_ENABLED  0x00
#define IMX708_LPF_INTENSITY_DISABLED 0x01
#define IMX708_LPF_INTENSITY          0xC429
#define IMX708_REG_EXPOSURE           0x0202
#define IMX708_EXPOSURE_DEFAULT       0x640
#define IMX708_EXPOSURE_SETTING       0x0A40

#define IMX708_SENSOR_I2CID           0x1A
#define IMX708_MIPI_CLOCK_FEQ         450  // MHz
#define IMX708_MIPI_LANE_CNT          2
#define IMX708_MIPI_DPP               10  // depth per pixel
#define IMX708_MIPITX_CNTCLK_EN       1   // continuous clock output enable
#define IMX708_LANE_NB                2

#define IMX708_SUB_SAMPLE             INP_SUBSAMPLE_DISABLE

#define IMX708_BINNING_0              INP_BINNING_4TO2_B
#define IMX708_BINNING_1              INP_BINNING_8TO2_B
#define IMX708_BINNING_2              INP_BINNING_16TO2_B

#define SENSORDPLIB_SENSOR_IMX708     SENSORDPLIB_SENSOR_HM2130
#define CIS_MIRROR_SETTING            0x03  // 0x00: off / 0x01:H-Mirror / 0x02:V-Mirror / 0x03:HV-Mirror
#define CIS_I2C_ID                    IMX708_SENSOR_I2CID
#define CIS_ENABLE_MIPI_INF           0x01  // 0x00: off / 0x01: on
#define CIS_MIPI_LANE_NUMBER          0x02
#define CIS_ENABLE_HX_AUTOI2C         0x00  // 0x00: off / 0x01: on / 0x2: on and XSLEEP KEEP HIGH

static bool _is_version_c = false;

static HX_CIS_SensorSetting_t IMX708_common_setting[] = {
#include "IMX708_common_setting.i"
};

static HX_CIS_SensorSetting_t IMX708_2304x1296_setting[] = {
#include "IMX708_mipi_2lane_2304x1296.i"
};

static HX_CIS_SensorSetting_t IMX708_stream_on[] = {
  {HX_CIS_I2C_Action_W, 0x0100, 0x01},
};

static HX_CIS_SensorSetting_t IMX708_stream_off[] = {
  {HX_CIS_I2C_Action_W, 0x0100, 0x00},
};

static HX_CIS_SensorSetting_t IMX708_link_450Mhz_regs[] = {
  {HX_CIS_I2C_Action_W, 0x030E, 0x01},
  {HX_CIS_I2C_Action_W, 0x030F, 0x2c},
};

static HX_CIS_SensorSetting_t IMX708_exposure_setting[] = {
  {HX_CIS_I2C_Action_W, 0x0202, ((IMX708_EXPOSURE_SETTING >> 8) & 0xFF)},
  {HX_CIS_I2C_Action_W, 0x0203,        (IMX708_EXPOSURE_SETTING & 0xFF)},
};

static HX_CIS_SensorSetting_t IMX708_gain_setting[] = {
  {HX_CIS_I2C_Action_W, 0x020e, 0x02},
  {HX_CIS_I2C_Action_W, 0x0204, 0x01},
};

static HX_CIS_SensorSetting_t IMX708_color_balance_setting[] = {
  {HX_CIS_I2C_Action_W, 0x0b90, 0xFF},
  {HX_CIS_I2C_Action_W, 0x0b92, 0xFF},
};

static const uint8_t pdaf_gains[2][9] = {
  {0x4c, 0x4c, 0x4c, 0x46, 0x3e, 0x38, 0x35, 0x35, 0x35},
  {0x35, 0x35, 0x35, 0x38, 0x3e, 0x46, 0x4c, 0x4c, 0x4c}
};

static HX_CIS_SensorSetting_t IMX708_mirror_setting[] = {
  {HX_CIS_I2C_Action_W, 0x0101, (CIS_MIRROR_SETTING & 0xFF)},
};

ma_err_t drv_imx708_probe() {
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
    MA_LOGD(MA_TAG, "IMX708 ID: %02X", data);

    return MA_OK;
}

static uint32_t imx708_set_pll200() {
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

    cfg.dpclk = SCU_HSCDPCLKSRC_RC96M48M;

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
    uint32_t bitrate_1lane  = IMX708_MIPI_CLOCK_FEQ * 2;
    uint32_t mipi_lnno      = IMX708_MIPI_LANE_CNT;
    uint32_t pixel_dpp      = IMX708_MIPI_DPP;
    uint32_t line_length    = IMX708_SENSOR_WIDTH;
    uint32_t frame_length   = IMX708_SENSOR_HEIGHT;
    uint32_t byte_clk       = bitrate_1lane / 8;
    uint32_t continuousout  = IMX708_MIPITX_CNTCLK_EN;
    uint32_t deskew_en      = 0;
    uint32_t mipi_pixel_clk = imx708_set_pll200();

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
    MA_LOGD(MA_TAG, "MIPI Disable");

    volatile uint32_t* dphy_reg                  = CSIRX_DPHY_REG;
    volatile uint32_t* csi_static_cfg_reg        = (CSIRX_REGS_BASE + 0x08);
    volatile uint32_t* csi_dphy_lane_control_reg = (CSIRX_REGS_BASE + 0x40);
    volatile uint32_t* csi_stream0_control_reg   = (CSIRX_REGS_BASE + 0x100);
    volatile uint32_t* csi_stream0_data_cfg      = (CSIRX_REGS_BASE + 0x108);
    volatile uint32_t* csi_stream0_cfg_reg       = (CSIRX_REGS_BASE + 0x10C);

    sensordplib_csirx_disable();

    MA_LOGD(MA_TAG, "MIPI DPHY: 0x%08X 0x%08X", dphy_reg, *dphy_reg);
    MA_LOGD(MA_TAG, "MIPI Static CFG: 0x%08X 0x%08X", csi_static_cfg_reg, *csi_static_cfg_reg);
    MA_LOGD(MA_TAG, "MIPI Lane Control: 0x%08X 0x%08X", csi_dphy_lane_control_reg, *csi_dphy_lane_control_reg);
    MA_LOGD(MA_TAG, "MIPI Stream Data CFG: 0x%08X 0x%08X", csi_stream0_data_cfg, *csi_stream0_data_cfg);
    MA_LOGD(MA_TAG, "MIPI Stream Control: 0x%08X 0x%08X", csi_stream0_control_reg, *csi_stream0_control_reg);
    MA_LOGD(MA_TAG, "MIPI Stream CFG: 0x%08X 0x%08X", csi_stream0_cfg_reg, *csi_stream0_cfg_reg);
}

static void _on_frame_ready_cb() {
    int ec = hx_drv_cis_setRegTable(IMX708_stream_off, HX_CIS_SIZE_N(IMX708_stream_off, HX_CIS_SensorSetting_t));
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "On frame ready fail: %d", ec);
    }
    set_mipi_csirx_disable();
}

static void _on_next_capture_cb() {
    set_mipi_csirx_enable();

    int ec = hx_drv_cis_setRegTable(IMX708_stream_on, HX_CIS_SIZE_N(IMX708_stream_on, HX_CIS_SensorSetting_t));
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "On stop capture fail: %d", ec);
    }
}

ma_err_t drv_imx708_init(uint16_t width, uint16_t height, int compression) {
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

        ec = hx_drv_cis_setRegTable(IMX708_stream_off, HX_CIS_SIZE_N(IMX708_stream_off, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Stream Off Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        ec =
          hx_drv_cis_setRegTable(IMX708_common_setting, HX_CIS_SIZE_N(IMX708_common_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Common Setting Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        uint8_t val = 0;
        ec          = hx_drv_cis_get_reg(IMX708_REG_BASE_SPC_GAINS_L, &val);
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGW(MA_TAG, "Camera Get SPC Gains Fail: %d", ec);
        } else if (val == 0x40) {
            MA_LOGD(MA_TAG, "Init PDAF Gains");
            for (uint32_t i = 0; i < 54 && ec == HX_CIS_NO_ERROR; i++) {
                ec = hx_drv_cis_set_reg(IMX708_REG_BASE_SPC_GAINS_L + i, pdaf_gains[0][i % 9], 0);
            }
            for (uint32_t i = 0; i < 54 && ec == HX_CIS_NO_ERROR; i++) {
                ec = hx_drv_cis_set_reg(IMX708_REG_BASE_SPC_GAINS_R + i, pdaf_gains[1][i % 9], 0);
            }
            MA_LOGD(MA_TAG, "Init PDAF Gains Done: %d", ec);
        }

        ec = hx_drv_cis_setRegTable(IMX708_2304x1296_setting,
                                    HX_CIS_SIZE_N(IMX708_2304x1296_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Resolution Setting Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        ec = hx_drv_cis_setRegTable(IMX708_exposure_setting,
                                    HX_CIS_SIZE_N(IMX708_exposure_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Exposure Setting Fail: %d", ec);
        }

        ec = hx_drv_cis_setRegTable(IMX708_gain_setting, HX_CIS_SIZE_N(IMX708_gain_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Gain Setting Fail: %d", ec);
        }

        ec = hx_drv_cis_setRegTable(IMX708_color_balance_setting,
                                    HX_CIS_SIZE_N(IMX708_color_balance_setting, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Color Balance Setting Fail: %d", ec);
        }

        ec = hx_drv_cis_setRegTable(IMX708_link_450Mhz_regs,
                                    HX_CIS_SIZE_N(IMX708_link_450Mhz_regs, HX_CIS_SensorSetting_t));
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Link 450Mhz Fail: %d", ec);
            ret = MA_EIO;
            goto err;
        }

        MA_LOGD(MA_TAG, "Quad Bayer re-mosaic adjustments %d", IMX708_QBC_ADJUST);
        if (IMX708_QBC_ADJUST > 0) {
            ec = hx_drv_cis_set_reg(IMX708_LPF_INTENSITY, IMX708_QBC_ADJUST, 0);
            if (ec != HX_CIS_NO_ERROR) {
                MA_LOGE(MA_TAG, "Camera Set Quad Bayer Fail: %d", ec);
            }
            ec = hx_drv_cis_set_reg(IMX708_LPF_INTENSITY_EN, IMX708_LPF_INTENSITY_ENABLED, 0);
        } else {
            ec = hx_drv_cis_set_reg(IMX708_LPF_INTENSITY_EN, IMX708_LPF_INTENSITY_DISABLED, 0);
        }
        if (ec != HX_CIS_NO_ERROR) {
            MA_LOGE(MA_TAG, "Camera Set Quad Bayer Fail: %d", ec);
        }

        ec =
          hx_drv_cis_setRegTable(IMX708_mirror_setting, HX_CIS_SIZE_N(IMX708_mirror_setting, HX_CIS_SensorSetting_t));
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

    int32_t factor_w = floor((float)IMX708_SENSOR_WIDTH / (float)width);
    int32_t factor_h = floor((float)IMX708_SENSOR_HEIGHT / (float)height);
    int32_t min_f    = factor_w < factor_h ? factor_w : factor_h;

    if (min_f >= 8) {
        w  = IMX708_SENSOR_WIDTH / 8;
        h  = IMX708_SENSOR_HEIGHT / 8;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX708,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX708_SENSOR_WIDTH,
                                                        IMX708_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_DISABLE,
                                                        crop,
                                                        INP_BINNING_16TO2_B);
    } else if (min_f >= 4) {
        w  = IMX708_SENSOR_WIDTH / 4;
        h  = IMX708_SENSOR_HEIGHT / 4;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX708,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX708_SENSOR_WIDTH,
                                                        IMX708_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_DISABLE,
                                                        crop,
                                                        INP_BINNING_8TO2_B);
    } else if (min_f >= 2) {
        w  = IMX708_SENSOR_WIDTH / 2;
        h  = IMX708_SENSOR_HEIGHT / 2;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX708,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX708_SENSOR_WIDTH,
                                                        IMX708_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_DISABLE,
                                                        crop,
                                                        INP_BINNING_4TO2_B);
    } else {
        w  = IMX708_SENSOR_WIDTH;
        h  = IMX708_SENSOR_HEIGHT;
        ec = sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX708,
                                                        SENSORDPLIB_STREAM_NONEAOS,
                                                        IMX708_SENSOR_WIDTH,
                                                        IMX708_SENSOR_HEIGHT,
                                                        INP_SUBSAMPLE_DISABLE,
                                                        crop,
                                                        INP_BINNING_DISABLE);
    }

    if (ec != 0) {
        MA_LOGE(MA_TAG, "Camera Set Sensor Control Fail: %d", ec);
        ret = MA_EIO;
        goto err;
    }

    start_x = (w - width) / 2;
    start_y = (h - height) / 2;

    MA_LOGD(MA_TAG, "Start X: %d Start Y: %d Width: %d Height: %d", start_x, start_y, width, height);

    _reset_all_wdma_buffer();

    _frame.width  = width;
    _frame.height = height;
    _frame.rotate = MA_PIXEL_ROTATE_0;
    _frame.format = MA_PIXEL_FORMAT_YUV422;
    _frame.size   = YUV422_SIZE_EXP(width, height);
    _frame.data   = _wdma3_baseaddr;
    _jpeg.width   = width;
    _jpeg.height  = height;
    _jpeg.rotate  = MA_PIXEL_ROTATE_0;
    _jpeg.format  = MA_PIXEL_FORMAT_JPEG;
    _jpeg.size    = JPEG_BASE_SIZE_EXP(width, height);
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
    hw5x5_cfg.demos_pattern_mode = DEMOS_PATTENMODE_BGGR;
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

        ec = hx_drv_cis_setRegTable(IMX708_stream_on, HX_CIS_SIZE_N(IMX708_stream_on, HX_CIS_SensorSetting_t));
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

    ec = sensordplib_set_sensorctrl_start();
    if (ec != 0) {
        MA_LOGE(MA_TAG, "Camera Set Sensor Control Start Fail: %d", ec);
        ret = MA_FAILED;
        goto err;
    }

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

    ec = hx_drv_cis_setRegTable(IMX708_stream_off, HX_CIS_SIZE_N(IMX708_stream_off, HX_CIS_SensorSetting_t));
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "Camera Set Stream Off Fail: %d", ec);
    }

    set_mipi_csirx_disable();
#endif

    hx_drv_sensorctrl_set_xSleep(1);

    return ret;
}

void drv_imx708_deinit() {
    MA_LOGD(MA_TAG, "Camera Deinit");

    int ec;

    sensordplib_stop_capture();

#if !ENABLE_SENSOR_FAST_SWITCH
    _frame_count = 0;

    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();

    ec = hx_drv_cis_setRegTable(IMX708_stream_off, HX_CIS_SIZE_N(IMX708_stream_off, HX_CIS_SensorSetting_t));
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "Camera Set Stream Off Fail: %d", ec);
    }
    set_mipi_csirx_disable();

#endif

    hx_drv_sensorctrl_set_xSleep(1);

    hx_drv_timer_cm55x_delay_ms(3, TIMER_STATE_DC);
}
