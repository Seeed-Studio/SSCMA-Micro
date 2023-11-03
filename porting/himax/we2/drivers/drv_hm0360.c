#include "drv_hm0360.h"

static HX_CIS_SensorSetting_t HM0360_init_setting[] = {
#include "HM0360_24MHz_Bayer_640x480_setA_VGA_setB_QVGA_MIPI_4b_ParallelOutput_R2.i"
};

static HX_CIS_SensorSetting_t HM0360_stream_on[] = {
  {HX_CIS_I2C_Action_W, 0x0100, 0x01},
};

static HX_CIS_SensorSetting_t HM0360_stream_off[] = {
  {HX_CIS_I2C_Action_W, 0x0100, 0x00},
};

static HX_CIS_SensorSetting_t HM0360_stream_xsleep[] = {
  {HX_CIS_I2C_Action_W, 0x0100, 0x02},
};

static volatile bool     _frame_ready       = false;
static volatile uint32_t _frame_count       = 0;
static volatile uint32_t _wdma1_baseaddr    = 0;
static volatile uint32_t _wdma2_baseaddr    = 0;
static volatile uint32_t _wdma3_baseaddr    = 0;
static volatile uint32_t _jpegsize_baseaddr = 0;
static el_img_t          _frame, _jpeg;

el_res_t _drv_hm0360_fit_res(uint16_t width, uint16_t height) {
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
    EL_LOGD("fit width: %d height: %d", res.width, res.height);
    return res;
}

void drv_hm0360_cb(SENSORDPLIB_STATUS_E event) {
    // EL_LOGD("Event: %d", event);
    switch (event) {
    case SENSORDPLIB_STATUS_XDMA_FRAME_READY:
        _frame_ready = true;
        _frame_count++;
        break;
    default:
        EL_LOGD("unkonw event");
        break;
    }
    return;
}

el_err_code_t drv_hm0360_init(uint16_t width, uint16_t height) {
    el_err_code_t ret = EL_OK;
    HW5x5_CFG_T   hw5x5_cfg;
    JPEG_CFG_T    jpeg_cfg;
    uint16_t      start_x, start_y;
    el_res_t      res;
    INP_CROP_T    crop;

    // check width * height
    if (width > HM0360_MAX_WIDTH || width > HM0360_MAX_HEIGHT) {
        return EL_EINVAL;
    }

    // pinmux
    hx_drv_scu_set_SEN_INT_pinmux(SCU_SEN_INT_PINMUX_FVALID);
    hx_drv_scu_set_SEN_GPIO_pinmux(SCU_SEN_GPIO_PINMUX_LVALID);
    hx_drv_scu_set_SEN_XSLEEP_pinmux(SCU_SEN_XSLEEP_PINMUX_SEN_XSLEEP_0);

    // clock
    hx_drv_dp_set_mclk_src(DP_MCLK_SRC_INTERNAL, DP_MCLK_SRC_INT_SEL_XTAL);

    // sleep control
    hx_drv_cis_init(DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV1);
    hx_drv_sensorctrl_set_xSleepCtrl(SENSORCTRL_XSLEEP_BY_CPU);

    // power on
    hx_drv_sensorctrl_set_xSleep(1);

    // off stream
    if (hx_drv_cis_setRegTable(HM0360_stream_off, HX_CIS_SIZE_N(HM0360_stream_off, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        ret = EL_EIO;
        goto err;
    }

    // init stream
    if (hx_drv_cis_setRegTable(HM0360_init_setting, HX_CIS_SIZE_N(HM0360_init_setting, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        ret = EL_EIO;
        goto err;
    }

    HX_CIS_SensorSetting_t HM0360_mirror_setting[] = {
      {HX_CIS_I2C_Action_W, 0x0101, 0x00},
    };

    if (hx_drv_cis_setRegTable(HM0360_mirror_setting, HX_CIS_SIZE_N(HM0360_mirror_setting, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        ret = EL_EIO;
        goto err;
    }

    res = _drv_hm0360_fit_res(width, height);

    start_x = (res.width - width) / 2;
    start_y = (res.height - height) / 2;

    EL_LOGD("start_x: %d start_y: %d width: %d height: %d", start_x, start_y, width, height);

    // DMA
    _jpegsize_baseaddr = (uint32_t)el_aligned_malloc_once(32, 64);
    if (_jpegsize_baseaddr == 0) {
        ret = EL_ENOMEM;
        goto err;
    }

    _wdma1_baseaddr = (uint32_t)el_aligned_malloc_once(32, res.width * res.height / 4);  // JPEG
    if (_wdma1_baseaddr == 0) {
        ret = EL_ENOMEM;
        goto err;
    }
    _wdma2_baseaddr = _wdma1_baseaddr;

    _wdma3_baseaddr = (uint32_t)el_aligned_malloc_once(32, res.width * res.height * 3 / 2);
    if (_wdma3_baseaddr == 0) {
        ret = EL_ENOMEM;
        goto err;
    }

    sensordplib_set_xDMA_baseaddrbyapp(_wdma1_baseaddr, _wdma2_baseaddr, _wdma3_baseaddr);
    sensordplib_set_jpegfilesize_addrbyapp(_jpegsize_baseaddr);

    EL_LOGD("WD1[%x], WD2_J[%x], WD3_RAW[%x], JPAuto[%x]",
            _wdma1_baseaddr,
            _wdma2_baseaddr,
            _wdma3_baseaddr,
            _jpegsize_baseaddr);

    _frame.data   = _wdma3_baseaddr;
    _frame.size   = width * height * 3 / 2;
    _frame.height = height;
    _frame.width  = width;
    _frame.rotate = EL_PIXEL_ROTATE_0;
    _frame.format = EL_PIXEL_FORMAT_YUV422;

    _jpeg.data   = _wdma2_baseaddr;
    _jpeg.size   = width * height / 4;
    _jpeg.height = height;
    _jpeg.width  = width;
    _jpeg.rotate = EL_PIXEL_ROTATE_0;
    _jpeg.format = EL_PIXEL_FORMAT_JPEG;

    // datapath init

    if (HM0360_MAX_WIDTH / res.width == 3) {
        sensordplib_set_sensorctrl_inp(SENSORDPLIB_SENSOR_HM0360_MODE3,
                                       SENSORDPLIB_STREAM_NONEAOS,
                                       HM0360_MAX_WIDTH,
                                       HM0360_MAX_HEIGHT,
                                       INP_SUBSAMPLE_8TO2);
        EL_LOGD("subsample: 4x");
    } else if (HM0360_MAX_WIDTH / res.width == 2) {
        sensordplib_set_sensorctrl_inp(SENSORDPLIB_SENSOR_HM0360_MODE3,
                                       SENSORDPLIB_STREAM_NONEAOS,
                                       HM0360_MAX_WIDTH,
                                       HM0360_MAX_HEIGHT,
                                       INP_SUBSAMPLE_4TO2);
        EL_LOGD("subsample: 2x");
    } else {
        sensordplib_set_sensorctrl_inp(SENSORDPLIB_SENSOR_HM0360_MODE3,
                                       SENSORDPLIB_STREAM_NONEAOS,
                                       HM0360_MAX_WIDTH,
                                       HM0360_MAX_HEIGHT,
                                       INP_SUBSAMPLE_DISABLE);
        EL_LOGD("subsample: 1x");
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
    jpeg_cfg.jpeg_encqtable = JPEG_ENC_QTABLE_4X;

    // sensordplib_set_int_hw5x5rgb_jpeg_wdma23(hw5x5_cfg, jpeg_cfg, 1, NULL);
    sensordplib_set_int_hw5x5_jpeg_wdma23(hw5x5_cfg, jpeg_cfg, 1, NULL);

    hx_dplib_register_cb(drv_hm0360_cb, SENSORDPLIB_CB_FUNTYPE_DP);

    if (hx_drv_cis_setRegTable(HM0360_stream_on, HX_CIS_SIZE_N(HM0360_stream_on, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        return EL_EIO;
    }

    sensordplib_set_mclkctrl_xsleepctrl_bySCMode();
    sensordplib_set_sensorctrl_start();

    EL_LOGD("hm0360 init success!");
    return ret;

err:
    // power off
    EL_LOGD("hm0360 init failed!");
    hx_drv_sensorctrl_set_xSleep(0);

    if (_jpegsize_baseaddr != 0) {
        el_free(_jpegsize_baseaddr);
        _jpegsize_baseaddr = 0;
    }
    if (_wdma3_baseaddr != 0) {
        el_free(_wdma3_baseaddr);
        _wdma3_baseaddr = 0;
    }
    if (_wdma1_baseaddr) {
        el_free(_wdma3_baseaddr);
        _wdma1_baseaddr = 0;
        _wdma2_baseaddr = 0;
    }
    return ret;
}

el_err_code_t drv_hm0360_deinit() {
    // datapath off
    sensordplib_stop_capture();
    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();
    // stream off
    if (hx_drv_cis_setRegTable(HM0360_stream_off, HX_CIS_SIZE_N(HM0360_stream_off, HX_CIS_SensorSetting_t)) !=
        HX_CIS_NO_ERROR) {
        return EL_EIO;
    }
    // power off
    hx_drv_sensorctrl_set_xSleep(0);
    return EL_OK;
}

el_err_code_t drv_hm0360_capture(uint32_t timeout) {
    uint32_t time = el_get_time_ms();
    _frame_ready  = false;

    sensordplib_retrigger_capture();

    while (!_frame_ready) {
        if (el_get_time_ms() - time >= timeout) {
            return EL_ETIMOUT;
        }
        el_sleep(5);
    }

    return EL_OK;
}

el_img_t drv_hm0360_get_frame() { return _frame; }

el_img_t drv_hm0360_get_jpeg() {
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
    return _jpeg;
}
