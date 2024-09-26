

#include "ma_transport_i2c.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>

extern "C" {
#include <WE2_core.h>
#include <WE2_device_addr.h>
#include <console_io.h>
#include <hx_drv_gpio.h>
#include <hx_drv_iic.h>
#include <hx_drv_scu.h>
}

#include <porting/ma_osal.h>

#include <core/utils/ma_ringbuffer.hpp>

#include "ma_config_board.h"

#define HEADER_LEN                      4
#define MAX_PL_LEN                      256
#define CHECKSUM_LEN                    2
#define ALIGNED_LEN                     26
#define MAX_PAYLOAD_LEN                 (HEADER_LEN + MAX_PL_LEN + CHECKSUM_LEN + ALIGNED_LEN)

#define FEATURE_TRANSPORT               0x10
#define FEATURE_TRANSPORT_CMD_STATUS    0x00
#define FEATURE_TRANSPORT_CMD_READ      0x01
#define FEATURE_TRANSPORT_CMD_WRITE     0x02
#define FEATURE_TRANSPORT_CMD_AVAILABLE 0x03
#define FEATURE_TRANSPORT_CMD_START     0x04
#define FEATURE_TRANSPORT_CMD_STOP      0x05
#define FEATURE_TRANSPORT_CMD_RESET     0x06

#define I2CS_TX_TIME_OUT_MS             1000

#define I2C_READ_ENABLE_F                       \
    std::memset(_rx_buf, 0, 32);                \
    hx_CleanDCache_by_Addr((void*)_rx_buf, 32); \
    hx_drv_i2cs_interrupt_read(                 \
      USE_DW_IIC_SLV_0, 0x60, reinterpret_cast<uint8_t*>(_rx_buf), 32, (void*)_i2c_s_callback_func_rx);

#define I2C_WRITE_ENABLE_F(size)                                                                          \
    hx_CleanDCache_by_Addr((void*)_tx_buf, size);                                                         \
    hx_drv_i2cs_interrupt_write(                                                                          \
      USE_DW_IIC_SLV_0, 0x60, reinterpret_cast<uint8_t*>(_tx_buf), size, (void*)_i2c_s_callback_func_tx); \
    _i2cs_tx_timer(I2CS_TX_TIME_OUT_MS);

namespace ma {

static SPSCRingBuffer<char>* _rb_rx  = nullptr;
static SPSCRingBuffer<char>* _rb_tx  = nullptr;
static char*                 _rx_buf = nullptr;
static char*                 _tx_buf = nullptr;
static Mutex                 _tx_mutex;
static HX_DRV_DEV_IIC*       _i2c       = nullptr;
static volatile bool         _is_opened = false;

static void _i2cs_tx_timeout_cb(void* data) { __NVIC_SystemReset(); }

static void _i2cs_tx_timer(uint32_t timeout_ms) {
    TIMER_CFG_T timer_cfg;
    timer_cfg.period = timeout_ms;
    timer_cfg.mode   = TIMER_MODE_ONESHOT;
    timer_cfg.ctrl   = TIMER_CTRL_CPU;
    timer_cfg.state  = TIMER_STATE_DC;

    hx_drv_timer_cm55s_stop();
    hx_drv_timer_cm55s_start(&timer_cfg, (Timer_ISREvent_t)_i2cs_tx_timeout_cb);
}

static void _i2c_s_callback_func_rx(void* param);

static void _i2c_s_callback_func_tx(void* param) {
    memset(_tx_buf, 0, 4096);

    I2C_READ_ENABLE_F

    hx_drv_timer_cm55s_stop();
}

static void _i2c_s_callback_func_rx(void* param) {
    uint8_t  feature   = _rx_buf[0];
    uint8_t  cmd       = _rx_buf[1];
    uint16_t len       = _rx_buf[2] << 8 | _rx_buf[3];
    uint16_t available = _rb_tx->size();
    if (len > MAX_PL_LEN || feature != FEATURE_TRANSPORT) {
        I2C_READ_ENABLE_F
        return;
    }
    switch (cmd) {
    case FEATURE_TRANSPORT_CMD_WRITE:
        _rb_rx->push(&_rx_buf[4], len);
        I2C_READ_ENABLE_F
        break;
    case FEATURE_TRANSPORT_CMD_READ:
        if (len > _rb_tx->size()) {
            len = _rb_tx->size();
        }
        _rb_tx->pop(&_tx_buf[0], len);
        I2C_WRITE_ENABLE_F(len)
        break;
    case FEATURE_TRANSPORT_CMD_AVAILABLE:
        _tx_buf[0] = available >> 8;
        _tx_buf[1] = available & 0xff;
        I2C_WRITE_ENABLE_F(2)
        break;
    case FEATURE_TRANSPORT_CMD_RESET:
        _rb_rx->clear();
        _rb_tx->clear();
        I2C_READ_ENABLE_F
        break;

    default:
        I2C_READ_ENABLE_F
        break;
    }
}

static void _i2c_s_callback_func_err(void* param) {}

static void _i2c_s_callback_func_sta(void* param) {}

I2C::I2C() : Transport(MA_TRANSPORT_I2C) {}

I2C::~I2C() { deInit(); }

ma_err_t I2C::init(const void* config) {
    if (_is_opened || m_initialized) {
        return MA_OK;
    }

    (void)config;

    hx_drv_scu_set_PA2_pinmux(SCU_PA2_PINMUX_SB_I2C_S_SCL_0, 1);
    hx_drv_scu_set_PA3_pinmux(SCU_PA3_PINMUX_SB_I2C_S_SDA_0, 1);

    if (IIC_ERR_OK != hx_drv_i2cs_init(USE_DW_IIC_SLV_0, BASE_ADDR_APB_I2C_SLAVE)) {
        return MA_EIO;
    }

    _i2c = hx_drv_i2cs_get_dev(USE_DW_IIC_SLV_0);
    if (_i2c == nullptr) {
        return MA_EIO;
    }

    _i2c->iic_control(DW_IIC_CMD_SET_TXCB, (void*)_i2c_s_callback_func_tx);
    _i2c->iic_control(DW_IIC_CMD_SET_RXCB, (void*)_i2c_s_callback_func_rx);
    _i2c->iic_control(DW_IIC_CMD_SET_ERRCB, (void*)_i2c_s_callback_func_err);
    _i2c->iic_control(DW_IIC_CMD_SLV_SET_SLV_ADDR, (void*)0x60);

    HX_DRV_DEV_IIC_INFO* iic_info_ptr = &(_i2c->iic_info);
    if (iic_info_ptr == nullptr) {
        return MA_EIO;
    }
    iic_info_ptr->extra = (void*)this;

    if (_rx_buf == nullptr) {
        _rx_buf = new (std::align_val_t{32}) char[32];
    }

    if (_tx_buf == nullptr) {
        _tx_buf = new (std::align_val_t{32}) char[4096];
    }

    if (_rb_rx == nullptr) {
        _rb_rx = new SPSCRingBuffer<char>(4096);
    }

    if (_rb_tx == nullptr) {
        _rb_tx = new SPSCRingBuffer<char>(48 * 1024);
    }

    if (!_rx_buf || !_tx_buf || !_rb_rx || !_rb_tx) {
        return MA_ENOMEM;
    }

    std::memset(_rx_buf, 0, 32);
    std::memset(_tx_buf, 0, 4096);

    _is_opened = m_initialized = true;

    I2C_READ_ENABLE_F

    return MA_OK;
}

void I2C::deInit() {
    if (!_is_opened || !m_initialized) {
        return;
    }

    if (_i2c) {
        _i2c->iic_control(DW_IIC_CMD_SET_TXCB, (void*)NULL);
        _i2c->iic_control(DW_IIC_CMD_SET_RXCB, (void*)NULL);
        _i2c->iic_control(DW_IIC_CMD_SET_ERRCB, (void*)NULL);

        hx_drv_i2cs_deinit(USE_DW_IIC_SLV_0);

        _i2c = nullptr;
    }

    if (_rb_rx) {
        delete _rb_rx;
        _rb_rx = nullptr;
    }

    if (_rb_tx) {
        delete _rb_tx;
        _rb_tx = nullptr;
    }

    if (_rx_buf) {
        delete[] _rx_buf;
        _rx_buf = nullptr;
    }

    if (_tx_buf) {
        delete[] _tx_buf;
        _tx_buf = nullptr;
    }

    _is_opened = m_initialized = false;
}

size_t I2C::available() const { return _rb_rx->size(); }

size_t I2C::send(const char* data, size_t length) {
    if (!m_initialized || data == nullptr || length == 0) {
        return 0;
    }

    Guard guard(_tx_mutex);

    return _rb_tx->push(data, length);
}

size_t I2C::flush() {
    if (!m_initialized) {
        return -1;
    }

    return 0;
}

size_t I2C::receive(char* data, size_t length) {
    if (!m_initialized || length == 0) {
        return 0;
    }

    return _rb_rx->pop(data, length);
}

size_t I2C::receiveIf(char* data, size_t length, char delimiter) {
    if (!m_initialized || length == 0) {
        return 0;
    }

    return _rb_rx->popIf(data, length, delimiter);
}

}  // namespace ma
