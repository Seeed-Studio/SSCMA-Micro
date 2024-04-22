#include "drv_imx219.h"

#include "drv_common.h"
#include "drv_shared_cfg.h"

#define CIS_MIRROR_SETTING    (0x03)  // 0x00: off/0x01:H-Mirror/0x02:V-Mirror/0x03:HV-Mirror
#define CIS_I2C_ID            IMX219_SENSOR_I2CID
#define CIS_ENABLE_MIPI_INF   (0x01)  // 0x00: off/0x01: on
#define CIS_MIPI_LANE_NUMBER  (0x02)
#define CIS_ENABLE_HX_AUTOI2C (0x00)  // 0x00: off/0x01: on/0x2: on and XSLEEP KEEP HIGH
#define DEAULT_XHSUTDOWN_PIN  AON_GPIO2

#define SENCTRL_SENSOR_TYPE   SENSORDPLIB_SENSOR_IMX219
#define SENCTRL_STREAM_TYPE   SENSORDPLIB_STREAM_NONEAOS
#define SENCTRL_SENSOR_WIDTH  IMX219_SENSOR_WIDTH
#define SENCTRL_SENSOR_HEIGHT IMX219_SENSOR_HEIGHT
#define SENCTRL_SENSOR_CH     3

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

static uint32_t imx219_set_pll200() {
    SCU_PDHSC_DPCLK_CFG_T cfg;

    hx_drv_scu_get_pdhsc_dpclk_cfg(&cfg);

    uint32_t pllfreq;
    hx_drv_swreg_aon_get_pllfreq(&pllfreq);

    if (pllfreq == 400000000) {
        cfg.mipiclk.hscmipiclksrc = SCU_HSCMIPICLKSRC_PLL;
        cfg.mipiclk.hscmipiclkdiv = 1;
    } else {
        cfg.mipiclk.hscmipiclksrc = SCU_HSCMIPICLKSRC_PLL;
        cfg.mipiclk.hscmipiclkdiv = 0;
    }

    hx_drv_scu_set_pdhsc_dpclk_cfg(cfg, 0, 1);

    uint32_t mipi_pixel_clk = 96;
    hx_drv_scu_get_freq(SCU_CLK_FREQ_TYPE_HSC_MIPI_RXCLK, &mipi_pixel_clk);
    mipi_pixel_clk = mipi_pixel_clk / 1000000;

    EL_LOGD("MIPI CLK change to PLL freq:(%d / %d)\n", pllfreq, (cfg.mipiclk.hscmipiclkdiv + 1));
    EL_LOGD("MIPI TX CLK: %dM\n", mipi_pixel_clk);

    return mipi_pixel_clk;
}

el_err_code_t drv_imx219_probe() {
    hx_drv_cis_init((CIS_XHSHUTDOWN_INDEX_E)DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV3);

    CONFIG_EL_CAMERA_PWR_CTRL_INIT_F;
    hx_drv_timer_cm55x_delay_ms(10, TIMER_STATE_DC);

    if (hx_drv_cis_set_slaveID(CIS_I2C_ID) != HX_CIS_NO_ERROR) {
        return EL_EIO;
    }

    uint8_t data;
    if (hx_drv_cis_get_reg(0x0000, &data) != HX_CIS_NO_ERROR) {
        return EL_EIO;
    }

    EL_LOGD("IMX219 I2C ID: 0x%04X", CIS_I2C_ID);
    return EL_OK;
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
    uint32_t mipi_pixel_clk = 96;

    mipi_pixel_clk = imx219_set_pll200();

    EL_LOGD("MIPI CSI Init Enable");
    EL_LOGD("MIPI TX CLK: %dM", mipi_pixel_clk);
    EL_LOGD("MIPI BITRATE 1LANE: %dM", bitrate_1lane);
    EL_LOGD("MIPI DATA LANE: %d", mipi_lnno);
    EL_LOGD("MIPI PIXEL DEPTH: %d", pixel_dpp);
    EL_LOGD("MIPI LINE LENGTH: %d", line_length);
    EL_LOGD("MIPI FRAME LENGTH: %d", frame_length);
    EL_LOGD("MIPI CONTINUOUSOUT: %d", continuousout);
    EL_LOGD("MIPI DESKEW: %d", deskew_en);

    uint32_t n_preload = 15;
    uint32_t l_header  = 4;
    uint32_t l_footer  = 2;

    double t_input   = (double)(l_header + line_length * pixel_dpp / 8 + l_footer) / (mipi_lnno * byte_clk) + 0.06;
    double t_output  = (double)line_length / mipi_pixel_clk;
    double t_preload = (double)(7 + (n_preload * 4) / mipi_lnno) / mipi_pixel_clk;

    double delta_t = t_input - t_output - t_preload;

    EL_LOGD("t_input: %dns", (uint32_t)(t_input * 1000));
    EL_LOGD("t_output: %dns", (uint32_t)(t_output * 1000));
    EL_LOGD("t_preload: %dns", (uint32_t)(t_preload * 1000));

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
    EL_LOGD("MIPI RX FIFO FILL: %d", rx_fifo_fill);
    EL_LOGD("MIPI TX FIFO FILL: %d", tx_fifo_fill);

    /*
	 * Reset CSI RX/TX
	 */
    EL_LOGD("RESET MIPI CSI RX/TX");

    SCU_DP_SWRESET_T dp_swrst;
    drv_interface_get_dp_swreset(&dp_swrst);
    dp_swrst.HSC_MIPIRX = 0;
    dp_swrst.HSC_MIPITX = 0;

    hx_drv_scu_set_DP_SWReset(dp_swrst);
    hx_drv_timer_cm55x_delay_us(50, TIMER_STATE_DC);

    dp_swrst.HSC_MIPIRX = 1;
    dp_swrst.HSC_MIPITX = 1;
    hx_drv_scu_set_DP_SWReset(dp_swrst);

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
        EL_LOGD("PIXEL DEPTH2 fail %d", pixel_dpp);
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
    hx_drv_scu_set_vmute(&ctrl);

    EL_LOGD("VMUTE: 0x%08X", *(uint32_t*)(SCU_LSC_ADDR + 0x408));
    EL_LOGD("0x53061000: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x1000));
    EL_LOGD("0x53061004: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x1004));
    EL_LOGD("0x53061008: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x1008));
    EL_LOGD("0x5306100C: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x100C));
    EL_LOGD("0x53061010: 0x%08X", *(uint32_t*)(CSITX_REGS_BASE + 0x1010));
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

#define WE2_CHIP_VERSION_C 0x8538000c

static void _on_frame_ready_cb() {
    // stream off
    if (hx_drv_cis_setRegTable(IMX219_stream_off, HX_CIS_SIZE_N(IMX219_stream_off, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        EL_LOGW("stream off fail");
    }
    set_mipi_csirx_disable();
}

static void _on_stop_capture_cb() {
    set_mipi_csirx_enable();
    // stream on
    if (hx_drv_cis_setRegTable(IMX219_stream_on, HX_CIS_SIZE_N(IMX219_stream_on, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        EL_LOGW("stream on fail");
    }
}

el_err_code_t drv_imx219_init(uint16_t width, uint16_t height) {
    uint32_t chipid;
    uint32_t version;
    hx_drv_scu_get_version(&chipid, &version);
    if (chipid == WE2_CHIP_VERSION_C) {
        _is_version_c     = true;
        _initiated_before = false;

        _drv_dp_event_cb_on_frame_ready = _on_frame_ready_cb;
        _drv_dp_on_stop_stream          = _on_stop_capture_cb;
    } else {
        _is_version_c                   = false;
        _drv_dp_event_cb_on_frame_ready = NULL;
        _drv_dp_on_stop_stream          = NULL;
    }
    el_err_code_t ret = EL_OK;
    HW5x5_CFG_T   hw5x5_cfg;
    JPEG_CFG_T    jpeg_cfg;
    uint16_t      start_x;
    uint16_t      start_y;
    el_res_t      res;
    INP_CROP_T    crop;

#if ENABLE_SENSOR_FAST_SWITCH
    if (!_initiated_before) {
        // BLOCK START
#endif
        // common CIS init
        hx_drv_cis_init((CIS_XHSHUTDOWN_INDEX_E)DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV3);
        EL_LOGD("mclk DIV3, xshutdown_pin=%d", DEAULT_XHSUTDOWN_PIN);

        // IMX219 Enable
        CONFIG_EL_CAMERA_PWR_CTRL_INIT_F;

        EL_LOGD("Init PA1(AON_GPIO1)");

        hx_drv_cis_set_slaveID(CIS_I2C_ID);
        EL_LOGD("hx_drv_cis_set_slaveID(0x%02X)", CIS_I2C_ID);

        el_sleep(3);

        // off stream
        if (hx_drv_cis_setRegTable(IMX219_stream_off, HX_CIS_SIZE_N(IMX219_stream_off, HX_CIS_SensorSetting_t)) !=
            HX_CIS_NO_ERROR) {
            ret = EL_EIO;
            goto err;
        }

        // init stream
        if (hx_drv_cis_setRegTable(IMX219_init_setting, HX_CIS_SIZE_N(IMX219_init_setting, HX_CIS_SensorSetting_t)) !=
            HX_CIS_NO_ERROR) {
            ret = EL_EIO;
            goto err;
        }

        if (hx_drv_cis_setRegTable(IMX219_binning_setting,
                                   HX_CIS_SIZE_N(IMX219_binning_setting, HX_CIS_SensorSetting_t)) != HX_CIS_NO_ERROR) {
            ret = EL_EIO;
            goto err;
        }

        if (hx_drv_cis_setRegTable(IMX219_exposure_setting,
                                   HX_CIS_SIZE_N(IMX219_exposure_setting, HX_CIS_SensorSetting_t)) != HX_CIS_NO_ERROR) {
            ret = EL_EIO;
            goto err;
        }

        if (hx_drv_cis_setRegTable(IMX219_again_setting, HX_CIS_SIZE_N(IMX219_again_setting, HX_CIS_SensorSetting_t)) !=
            HX_CIS_NO_ERROR) {
            ret = EL_EIO;
            goto err;
        }

        if (hx_drv_cis_setRegTable(IMX219_dgain_setting, HX_CIS_SIZE_N(IMX219_dgain_setting, HX_CIS_SensorSetting_t)) !=
            HX_CIS_NO_ERROR) {
            ret = EL_EIO;
            goto err;
        }

        if (hx_drv_cis_setRegTable(IMX219_mirror_setting,
                                   HX_CIS_SIZE_N(IMX219_mirror_setting, HX_CIS_SensorSetting_t)) != HX_CIS_NO_ERROR) {
            ret = EL_EIO;
            goto err;
        }

        set_mipi_csirx_enable();

#if ENABLE_SENSOR_FAST_SWITCH
        // BLOCK END
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
        res.width  = IMX219_SENSOR_WIDTH / 16;
        res.height = IMX219_SENSOR_HEIGHT / 16;

        sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                   SENSORDPLIB_STREAM_NONEAOS,
                                                   IMX219_SENSOR_WIDTH,
                                                   IMX219_SENSOR_HEIGHT,
                                                   INP_SUBSAMPLE_4TO2,
                                                   crop,
                                                   INP_BINNING_16TO2_B);
    } else if (min_f >= 8) {
        res.width  = IMX219_SENSOR_WIDTH / 8;
        res.height = IMX219_SENSOR_HEIGHT / 8;

        sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                   SENSORDPLIB_STREAM_NONEAOS,
                                                   IMX219_SENSOR_WIDTH,
                                                   IMX219_SENSOR_HEIGHT,
                                                   INP_SUBSAMPLE_DISABLE,
                                                   crop,
                                                   INP_BINNING_16TO2_B);
    } else if (min_f >= 5) {
        res.width  = IMX219_SENSOR_WIDTH / 5;
        res.height = IMX219_SENSOR_HEIGHT / 5;

        sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                   SENSORDPLIB_STREAM_NONEAOS,
                                                   IMX219_SENSOR_WIDTH,
                                                   IMX219_SENSOR_HEIGHT,
                                                   INP_SUBSAMPLE_DISABLE,
                                                   crop,
                                                   INP_BINNING_10TO2_B);
    } else if (min_f >= 4) {
        res.width  = IMX219_SENSOR_WIDTH / 4;
        res.height = IMX219_SENSOR_HEIGHT / 4;

        sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                   SENSORDPLIB_STREAM_NONEAOS,
                                                   IMX219_SENSOR_WIDTH,
                                                   IMX219_SENSOR_HEIGHT,
                                                   INP_SUBSAMPLE_DISABLE,
                                                   crop,
                                                   INP_BINNING_8TO2_B);
    } else {
        res.width  = IMX219_SENSOR_WIDTH;
        res.height = IMX219_SENSOR_HEIGHT;

        sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_IMX219,
                                                   SENSORDPLIB_STREAM_NONEAOS,
                                                   IMX219_SENSOR_WIDTH,
                                                   IMX219_SENSOR_HEIGHT,
                                                   INP_SUBSAMPLE_DISABLE,
                                                   crop,
                                                   INP_SUBSAMPLE_DISABLE);
    }

    start_x = (res.width - width) / 2;
    start_y = (res.height - height) / 2;

    EL_LOGD("start_x: %d start_y: %d width: %d height: %d", start_x, start_y, width, height);

    _frame.width  = width;
    _frame.height = height;
    _frame.rotate = EL_PIXEL_ROTATE_0;
    _frame.format = EL_PIXEL_FORMAT_YUV422;
    _frame.size   = width * height * 3 / 2;

    _jpeg.width  = width;
    _jpeg.height = height;
    _jpeg.rotate = EL_PIXEL_ROTATE_0;
    _jpeg.format = EL_PIXEL_FORMAT_JPEG;
    _jpeg.size   = width * height / 4;

    // DMA
    _reset_all_wdma_buffer();

    _frame.data = _wdma3_baseaddr;
    _jpeg.data  = _wdma2_baseaddr;

    EL_LOGD("wdma1[%x], wdma2[%x], wdma3[%x], jpg_sz[%x]",
            _wdma1_baseaddr,
            _wdma2_baseaddr,
            _wdma3_baseaddr,
            _jpegsize_baseaddr);

#if ENABLE_SENSOR_FAST_SWITCH
    if (!_initiated_before) {
        // BLOCK START
#endif
        sensordplib_set_xDMA_baseaddrbyapp(_wdma1_baseaddr, _wdma2_baseaddr, _wdma3_baseaddr);
        sensordplib_set_jpegfilesize_addrbyapp(_jpegsize_baseaddr);

#if ENABLE_SENSOR_FAST_SWITCH
        // BLOCK END
    }
#endif

    //HW5x5 Cfg
    hw5x5_cfg.hw5x5_path         = HW5x5_PATH_THROUGH_DEMOSAIC;
    hw5x5_cfg.demos_bndmode      = DEMOS_BNDODE_REFLECT;
    hw5x5_cfg.demos_color_mode   = DEMOS_COLORMODE_YUV422;
    hw5x5_cfg.demos_pattern_mode = DEMOS_PATTENMODE_BGGR;
    hw5x5_cfg.demoslpf_roundmode = DEMOSLPF_ROUNDMODE_ROUNDING;
    hw5x5_cfg.hw55_crop_stx      = start_x;
    hw5x5_cfg.hw55_crop_sty      = start_y;
    hw5x5_cfg.hw55_in_width      = width;
    hw5x5_cfg.hw55_in_height     = height;

    //JPEG Cfg
    jpeg_cfg.jpeg_path      = JPEG_PATH_ENCODER_EN;
    jpeg_cfg.enc_width      = width;
    jpeg_cfg.enc_height     = height;
    jpeg_cfg.jpeg_enctype   = JPEG_ENC_TYPE_YUV422;
    jpeg_cfg.jpeg_encqtable = JPEG_ENC_QTABLE_10X;

#if defined(CONFIG_EL_BOARD_DEV_BOARD_WE2)
    if (width > 240 && height > 240) {
        jpeg_cfg.jpeg_encqtable = JPEG_ENC_QTABLE_10X;
    } else {
        jpeg_cfg.jpeg_encqtable = JPEG_ENC_QTABLE_4X;
    }
#endif

    // sensordplib_set_int_hw5x5rgb_jpeg_wdma23(hw5x5_cfg, jpeg_cfg, 1, NULL);
    sensordplib_set_int_hw5x5_jpeg_wdma23(hw5x5_cfg, jpeg_cfg, 1, NULL);

#if ENABLE_SENSOR_FAST_SWITCH
    if (!_initiated_before) {
        // BLOCK START
#endif

        hx_dplib_register_cb(_drv_dp_event_cb, SENSORDPLIB_CB_FUNTYPE_DP);

        if (hx_drv_cis_setRegTable(IMX219_stream_on, HX_CIS_SIZE_N(IMX219_stream_on, HX_CIS_SensorSetting_t)) !=
            HX_CIS_NO_ERROR) {
            return EL_EIO;
        }

        sensordplib_set_mclkctrl_xsleepctrl_bySCMode();

#if ENABLE_SENSOR_FAST_SWITCH
        // BLOCK END
    }
#endif

    if (_is_version_c) {
        _on_stop_capture_cb();
    }

    sensordplib_set_sensorctrl_start();

    _frame_ready = false;
    sensordplib_retrigger_capture();

    if (_is_version_c) {
        auto last_time = el_get_time_ms();
        while (!_frame_ready) {
            el_sleep(3);
            if (el_get_time_ms() - last_time > 1000) {
                ret = EL_ETIMOUT;
                EL_LOGD("wait frame ready timeout");
                goto err;
            }
        }
        _on_stop_capture_cb();
    }

    _initiated_before = true;

    EL_LOGD("imx219 init success!");

    return ret;

err:
    // power off
    EL_LOGD("imx219 init failed!");

#if ENABLE_SENSOR_FAST_SWITCH
    _initiated_before = false;

    sensordplib_stop_capture();
    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();

    // stream off
    hx_drv_cis_setRegTable(IMX219_stream_off, HX_CIS_SIZE_N(IMX219_stream_off, HX_CIS_SensorSetting_t));

    set_mipi_csirx_disable();

#endif

    hx_drv_sensorctrl_set_xSleep(1);

    return ret;
}

el_err_code_t drv_imx219_deinit() {
    // datapath off
    sensordplib_stop_capture();

#if !ENABLE_SENSOR_FAST_SWITCH

    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();

    // stream off
    if (hx_drv_cis_setRegTable(IMX219_stream_off, HX_CIS_SIZE_N(IMX219_stream_off, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        return EL_EIO;
    }
    set_mipi_csirx_disable();

#endif

    // power off
    hx_drv_sensorctrl_set_xSleep(1);

    el_sleep(3);

    return EL_OK;
}
