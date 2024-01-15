#include "drv_ov5647.h"

static HX_CIS_SensorSetting_t OV5647_init_setting[] = {
#if (OV5647_MIPI_MODE == OV5647_MIPI_640X480)
    #include "OV5647_mipi_2lane_640x480.i"
#elif (OV5647_MIPI_MODE == OV5647_MIPI_2592X1944)
    #include "OV5647_mipi_2lane_2592x1944.i"
#elif (OV5647_MIPI_MODE == OV5647_MIPI_1296X972)
    #include "OV5647_mipi_2lane_1296x972.i"
#endif

};

static HX_CIS_SensorSetting_t OV5647_stream_on[] = {
  {HX_CIS_I2C_Action_W, 0x4800, OV5647_MIPI_CTRL_ON},
  {HX_CIS_I2C_Action_W, 0x4202,                0x00},
};

static HX_CIS_SensorSetting_t OV5647_stream_off[] = {
  {HX_CIS_I2C_Action_W, 0x4800, OV5647_MIPI_CTRL_OFF},
  {HX_CIS_I2C_Action_W, 0x4202,                 0x0F},
};

static volatile bool     _frame_ready       = false;
static volatile uint32_t _frame_count       = 0;
static volatile uint32_t _wdma1_baseaddr    = 0;
static volatile uint32_t _wdma2_baseaddr    = 0;
static volatile uint32_t _wdma3_baseaddr    = 0;
static volatile uint32_t _jpegsize_baseaddr = 0;
static el_img_t          _frame, _jpeg;

el_res_t _drv_ov5647_fit_res(uint16_t width, uint16_t height) {
    el_res_t res;
    if (width > 320 || height > 240) {
        res.width  = 640;
        res.height = 480;
    } else if (width > 160 || height > 120) {
        res.width  = 320;
        res.height = 240;
    } else {
        res.width  = 160;
        res.height = 120;
    }
    // if (width > 480 || height > 320) {
    //     res.width  = 960;
    //     res.height = 540;
    // } else if (width > 240 || height > 160) {
    //     res.width  = 480;
    //     res.height = 270;
    // } else {
    //     res.width  = 240;
    //     res.height = 135;
    // }
    EL_LOGD("fit width: %d height: %d", res.width, res.height);

    return res;
}

void drv_ov5647_cb(SENSORDPLIB_STATUS_E event) {
    // EL_LOGD("Event: %d", event);
    switch (event) {
    case SENSORDPLIB_STATUS_XDMA_FRAME_READY:
        _frame_ready = true;
        _frame_count++;
        break;
    default:
        el_printf("Unkonw event:%d", event);

        break;
    }
    return;
}

void set_mipi_csirx_enable() {
    uint32_t mipi_pixel_clk = 96;
    hx_drv_scu_get_freq(SCU_CLK_FREQ_TYPE_HSC_MIPI_RXCLK, &mipi_pixel_clk);

    mipi_pixel_clk = mipi_pixel_clk / 1000000;

    uint32_t bitrate_1lane = OV5647_MIPI_CLOCK_FEQ * 2;
    uint32_t mipi_lnno     = OV5647_MIPI_LANE_CNT;
    uint32_t pixel_dpp     = OV5647_MIPI_DPP;
    uint32_t line_length   = OV5647_SENSOR_WIDTH;
    uint32_t frame_length  = OV5647_SENSOR_HEIGHT;
    uint32_t byte_clk      = bitrate_1lane / 8;
    uint32_t continuousout = OV5647_MIPITX_CNTCLK_EN;
    uint32_t deskew_en     = 0;

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

    if (mipi_pixel_clk == 200)  //pll200
    {
        hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
        hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x10;
        hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x10;
    } else if (mipi_pixel_clk == 300)  //pll300
    {
        hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
        hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x18;
        hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x18;
    } else  //rc96
    {
        hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
        hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x06;
        hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x06;
    }
    sensordplib_csirx_set_hscnt(hscnt_cfg);

    if (pixel_dpp == 10 || pixel_dpp == 8) {
        sensordplib_csirx_set_pixel_depth(pixel_dpp);
    } else {
        EL_LOGD("PIXEL DEPTH fail %d", pixel_dpp);

        return;
    }

    sensordplib_csirx_set_deskew(deskew_en);
    sensordplib_csirx_set_fifo_fill(rx_fifo_fill);
    sensordplib_csirx_enable(mipi_lnno);

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

el_err_code_t drv_ov5647_init(uint16_t width, uint16_t height) {
    el_err_code_t ret = EL_OK;
    HW5x5_CFG_T   hw5x5_cfg;
    JPEG_CFG_T    jpeg_cfg;
    uint16_t      start_x, start_y;
    el_res_t      res;
    INP_CROP_T    crop;

    /*
     * common CIS init
     */
    hx_drv_cis_init((CIS_XHSHUTDOWN_INDEX_E)DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV3);
    EL_LOGD("mclk DIV3, xshutdown_pin=%d", DEAULT_XHSUTDOWN_PIN);

    //OV5647 Enable
    hx_drv_gpio_set_output(AON_GPIO1, GPIO_OUT_HIGH);
    hx_drv_scu_set_PA1_pinmux(SCU_PA1_PINMUX_AON_GPIO1, 0);
    hx_drv_gpio_set_out_value(AON_GPIO1, GPIO_OUT_HIGH);
    EL_LOGD("Set PA1(AON_GPIO1) to High");

    hx_drv_cis_set_slaveID(CIS_I2C_ID);
    EL_LOGD("hx_drv_cis_set_slaveID(0x%02X)", CIS_I2C_ID);

    el_sleep(3);
    // off stream
    if (hx_drv_cis_setRegTable(OV5647_stream_off, HX_CIS_SIZE_N(OV5647_stream_off, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        ret = EL_EIO;
        goto err;
    }

    // init stream
    if (hx_drv_cis_setRegTable(OV5647_init_setting, HX_CIS_SIZE_N(OV5647_init_setting, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        ret = EL_EIO;
        goto err;
    }

    HX_CIS_SensorSetting_t OV5647_mirror_setting[] = {
      {HX_CIS_I2C_Action_W, 0x0101, 0x00},
    };

    if (hx_drv_cis_setRegTable(OV5647_mirror_setting, HX_CIS_SIZE_N(OV5647_mirror_setting, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        ret = EL_EIO;
        goto err;
    }

    res = _drv_ov5647_fit_res(width, height);

    start_x = (res.width - width) / 2;
    start_y = (res.height - height) / 2;

    EL_LOGD("start_x: %d start_y: %d width: %d height: %d", start_x, start_y, width, height);

    set_mipi_csirx_enable();

    _frame.height = height;
    _frame.width  = width;
    _frame.rotate = EL_PIXEL_ROTATE_0;
    _frame.format = EL_PIXEL_FORMAT_YUV422;
    _frame.size   = width * height * 3 / 2;

    _jpeg.height = height;
    _jpeg.width  = width;
    _jpeg.rotate = EL_PIXEL_ROTATE_0;
    _jpeg.format = EL_PIXEL_FORMAT_JPEG;
    _jpeg.size   = width * height / 4;

    // DMA
    if (!_jpegsize_baseaddr) _jpegsize_baseaddr = (uint32_t)el_aligned_malloc_once(32, 64);
    if (_jpegsize_baseaddr == 0) {
        ret = EL_ENOMEM;
        goto err;
    }

    {
        size_t bs = (((623 + (size_t)(res.width / 16) * (size_t)(res.height / 16) * 128 + 35) >> 2) << 2);
        if (!_wdma1_baseaddr) _wdma1_baseaddr = (uint32_t)el_aligned_malloc_once(32, bs);  // JPEG
    }
    if (_wdma1_baseaddr == 0) {
        ret = EL_ENOMEM;
        goto err;
    }
    _wdma2_baseaddr = _wdma1_baseaddr;
    if (!_wdma3_baseaddr) _wdma3_baseaddr = (uint32_t)el_aligned_malloc_once(32, res.width * res.height * 3 / 2);
    if (_wdma3_baseaddr == 0) {
        ret = EL_ENOMEM;
        goto err;
    }

    _frame.data = _wdma3_baseaddr;
    _jpeg.data  = _wdma2_baseaddr;

    EL_LOGD("WD1[%x], WD2_J[%x], WD3_RAW[%x], JPAuto[%x]",
            _wdma1_baseaddr,
            _wdma2_baseaddr,
            _wdma3_baseaddr,
            _jpegsize_baseaddr);

    sensordplib_set_xDMA_baseaddrbyapp(_wdma1_baseaddr, _wdma2_baseaddr, _wdma3_baseaddr);
    sensordplib_set_jpegfilesize_addrbyapp(_jpegsize_baseaddr);

    crop.start_x = 0;
    crop.start_y = 0;
    crop.last_x = 0;
    crop.last_y = 0;


    switch (res.width) {
    case 640:
        sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_OV5647,
                                                   SENSORDPLIB_STREAM_NONEAOS,
                                                   OV5647_SENSOR_WIDTH,
                                                   OV5647_SENSOR_HEIGHT,
                                                   OV5647_SUB_SAMPLE,
                                                   crop,
                                                   INP_BINNING_DISABLE);
        break;
    case 320:
        sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_OV5647,
                                                   SENSORDPLIB_STREAM_NONEAOS,
                                                   OV5647_SENSOR_WIDTH,
                                                   OV5647_SENSOR_HEIGHT,
                                                   OV5647_SUB_SAMPLE,
                                                   crop,
                                                   INP_BINNING_4TO2_B);
        break;
    case 160:
        sensordplib_set_sensorctrl_inp_wi_crop_bin(SENSORDPLIB_SENSOR_OV5647,
                                                   SENSORDPLIB_STREAM_NONEAOS,
                                                   OV5647_SENSOR_WIDTH,
                                                   OV5647_SENSOR_HEIGHT,
                                                   OV5647_SUB_SAMPLE,
                                                   crop,
                                                   INP_BINNING_8TO2_B);
        break;

    default:
        break;
    }

    //HW5x5 Cfg
    hw5x5_cfg.hw5x5_path         = HW5x5_PATH_THROUGH_DEMOSAIC;
    hw5x5_cfg.demos_bndmode      = DEMOS_BNDODE_REFLECT;
    hw5x5_cfg.demos_color_mode   = DEMOS_COLORMODE_YUV422;
    hw5x5_cfg.demos_pattern_mode = DEMOS_PATTENMODE_BGGR;
    hw5x5_cfg.demoslpf_roundmode = DEMOSLPF_ROUNDMODE_FLOOR;
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

    // sensordplib_set_int_hw5x5rgb_jpeg_wdma23(hw5x5_cfg, jpeg_cfg, 1, NULL);
    sensordplib_set_int_hw5x5_jpeg_wdma23(hw5x5_cfg, jpeg_cfg, 1, NULL);

    hx_dplib_register_cb(drv_ov5647_cb, SENSORDPLIB_CB_FUNTYPE_DP);

    if (hx_drv_cis_setRegTable(OV5647_stream_on, HX_CIS_SIZE_N(OV5647_stream_on, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        return EL_EIO;
    }

    sensordplib_set_mclkctrl_xsleepctrl_bySCMode();
    sensordplib_set_sensorctrl_start();
    sensordplib_retrigger_capture();

    EL_LOGD("ov5647 init success!");

    return ret;

err:
    // power off
    EL_LOGD("ov5647 init failed!");

    hx_drv_sensorctrl_set_xSleep(0);

    // if (_jpegsize_baseaddr != 0) {
    //     el_free(_jpegsize_baseaddr);
    //     _jpegsize_baseaddr = 0;
    // }
    // if (_wdma3_baseaddr != 0) {
    //     el_free(_wdma3_baseaddr);
    //     _wdma3_baseaddr = 0;
    // }
    // if (_wdma1_baseaddr) {
    //     el_free(_wdma3_baseaddr);
    //     _wdma1_baseaddr = 0;
    //     _wdma2_baseaddr = 0;
    // }
    return ret;
}

el_err_code_t drv_ov5647_deinit() {
    // datapath off
    sensordplib_stop_capture();
    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();

    // stream off
    if (hx_drv_cis_setRegTable(OV5647_stream_off, HX_CIS_SIZE_N(OV5647_stream_off, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        return EL_EIO;
    }
    set_mipi_csirx_disable();
    // power off
    hx_drv_sensorctrl_set_xSleep(1);
    return EL_OK;
}

el_err_code_t drv_ov5647_capture(uint32_t timeout) {
    uint32_t time = el_get_time_ms();
    while (!_frame_ready) {
        if (el_get_time_ms() - time >= timeout) {
            return EL_ETIMOUT;
        }
        el_sleep(1);
    }

    return EL_OK;
}

el_err_code_t drv_ov5647_capture_stop() {
    _frame_ready = false;
    sensordplib_retrigger_capture();

    return EL_OK;
}

el_img_t drv_ov5647_get_frame() { return _frame; }

el_img_t drv_ov5647_get_jpeg() {
    uint8_t  frame_no, buffer_no = 0;
    uint32_t reg_val = 0, mem_val = 0;
    hx_drv_xdma_get_WDMA2_bufferNo(&buffer_no);
    hx_drv_xdma_get_WDMA2NextFrameIdx(&frame_no);
    if (frame_no == 0) {
        frame_no = buffer_no - 1;
    } else {
        frame_no = frame_no - 1;
    }
    hx_drv_jpeg_get_EncOutRealMEMSize(&reg_val);
    hx_drv_jpeg_get_FillFileSizeToMem(frame_no, (uint32_t)_jpegsize_baseaddr, &mem_val);
    hx_drv_jpeg_get_MemAddrByFrameNo(frame_no, _wdma2_baseaddr, &_jpeg.data);
    _jpeg.size = mem_val == reg_val ? mem_val : reg_val;
    hx_InvalidateDCache_by_Addr((volatile void*)_jpeg.data, _jpeg.size);
    EL_LOGD("JPEG size: %d", _jpeg.size);
    return _jpeg;
}
