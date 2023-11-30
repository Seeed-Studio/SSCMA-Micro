/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "el_wire_we2.h"

#include <cctype>
#include <cstdint>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "el_config_porting.h"

namespace edgelab {

namespace porting {

static void i2c_s_callback_fun_tx(void* param) {
    // el_printf("%s \n", __FUNCTION__);

    HX_DRV_DEV_IIC*      iic_obj      = (HX_DRV_DEV_IIC*)param;
    HX_DRV_DEV_IIC_INFO* iic_info_ptr = &(iic_obj->iic_info);
    WireWE2*             wire         = (WireWE2*)iic_info_ptr->extra;
    memset(wire->tx_buffer, 0, sizeof(wire->tx_buffer));
    wire->wire_read_enable(sizeof(wire->rx_buffer));
}

static void i2c_s_callback_fun_rx(void* param) {
    //el_printf("%s \n", __FUNCTION__);

    HX_DRV_DEV_IIC*      iic_obj      = (HX_DRV_DEV_IIC*)param;
    HX_DRV_DEV_IIC_INFO* iic_info_ptr = &(iic_obj->iic_info);

    WireWE2* wire = (WireWE2*)iic_info_ptr->extra;

    uint8_t  feature    = wire->rx_buffer[0];
    uint8_t  cmd        = wire->rx_buffer[1];
    uint16_t len        = wire->rx_buffer[2] << 8 | wire->rx_buffer[3];
    uint16_t crc        = wire->rx_buffer[4 + len] << 8 | wire->rx_buffer[5 + len];
    uint16_t available  = wire->tx_ring_buffer->size();
    bool     is_present = *wire;

    // el_printf("feature: %0x, cmd: %0x, len: %d, crc: %0x, available: %d\n", feature, cmd, len, crc, available);

    if (len > MAX_PL_LEN) {
        wire->wire_read_enable(sizeof(wire->rx_buffer));
        return;
    }

    if (FEATURE_TRANSPORT == feature) {
        switch (cmd) {
        case FEATURE_TRANSPORT_CMD_WRITE:
            if (is_present) {
                wire->rx_ring_buffer->push((char*)&wire->rx_buffer[4], len);
            }
            wire->wire_read_enable(sizeof(wire->rx_buffer));
            break;
        case FEATURE_TRANSPORT_CMD_READ:
            if (len > wire->tx_ring_buffer->size()) {
                len = wire->tx_ring_buffer->size();
            }
            wire->tx_ring_buffer->pop((char*)&wire->tx_buffer[0], len);
            wire->wire_write_enable(len);
            break;
        case FEATURE_TRANSPORT_CMD_AVAILABLE:
            wire->tx_buffer[0] = available >> 8;
            wire->tx_buffer[1] = available & 0xff;
            wire->wire_write_enable(2);
            break;
        case FEATURE_TRANSPORT_CMD_RESET:
            wire->rx_ring_buffer->clear();
            wire->tx_ring_buffer->clear();
            wire->wire_read_enable(sizeof(wire->rx_buffer));
            break;
        case FEATURE_TRANSPORT_CMD_START:
            wire->set_present(true);
            wire->wire_read_enable(sizeof(wire->rx_buffer));
            break;
        case FEATURE_TRANSPORT_CMD_STOP:
            wire->set_present(false);
            wire->wire_read_enable(sizeof(wire->rx_buffer));
            break;
        case FEATURE_TRANSPORT_CMD_STATUS:
            wire->tx_buffer[0] = is_present;
            wire->wire_write_enable(1);
            break;
        default:
            wire->wire_read_enable(sizeof(wire->rx_buffer));
            break;
        }

    } else {
        wire->wire_read_enable(sizeof(wire->rx_buffer));
    }
}

static void i2c_s_callback_fun_err(void* param) {
    //el_printf("%s \n", __FUNCTION__);
    HX_DRV_DEV_IIC*      iic_obj      = (HX_DRV_DEV_IIC*)param;
    HX_DRV_DEV_IIC_INFO* iic_info_ptr = &(iic_obj->iic_info);

    WireWE2* wire = (WireWE2*)iic_info_ptr->extra;

    wire->wire_read_enable(sizeof(wire->rx_buffer));
}
static void i2c_s_callback_fun_sta(void* param) {
    //el_printf("%s \n", __FUNCTION__);

    HX_DRV_DEV_IIC*      iic_obj      = (HX_DRV_DEV_IIC*)param;
    HX_DRV_DEV_IIC_INFO* iic_info_ptr = &(iic_obj->iic_info);
}

}  // namespace porting

using namespace porting;

WireWE2::WireWE2(uint8_t addr) : Wire{addr} {}

WireWE2::~WireWE2() { deinit(); }

el_err_code_t WireWE2::init() {
    EL_ASSERT(!this->_is_present);

    this->index = USE_DW_IIC_SLV_0;
    hx_drv_scu_set_PA2_pinmux(SCU_PA2_PINMUX_SB_I2C_S_SCL_0);
    hx_drv_scu_set_PA3_pinmux(SCU_PA3_PINMUX_SB_I2C_S_SDA_0);

    if (IIC_ERR_OK != hx_drv_i2cs_init((USE_DW_IIC_SLV_E)this->index, HX_I2C_HOST_SLV_0_BASE)) {
        return EL_EIO;
    }

    this->i2c = hx_drv_i2cs_get_dev((USE_DW_IIC_SLV_E)this->index);

    this->i2c->iic_control(DW_IIC_CMD_SET_TXCB, (void*)i2c_s_callback_fun_tx);
    this->i2c->iic_control(DW_IIC_CMD_SET_RXCB, (void*)i2c_s_callback_fun_rx);
    this->i2c->iic_control(DW_IIC_CMD_SET_ERRCB, (void*)i2c_s_callback_fun_err);
    //this->i2c->iic_control(DW_IIC_CMD_SET_STACB, (void*)i2c_s_callback_fun_sta);
    this->i2c->iic_control(DW_IIC_CMD_SLV_SET_SLV_ADDR, (void*)this->addr);

    HX_DRV_DEV_IIC_INFO* iic_info_ptr = &(this->i2c->iic_info);
    iic_info_ptr->extra               = (void*)this;

    this->_is_init    = this->i2c != nullptr;
    this->_is_present = false;

    this->rx_ring_buffer = new lwRingBuffer(512);
    this->tx_ring_buffer = new lwRingBuffer(16384);

    this->wire_read_enable(sizeof(this->tx_buffer));

    return this->_is_init ? EL_OK : EL_EIO;
}

el_err_code_t WireWE2::deinit() {
    if (this->i2c == nullptr) {
        return EL_OK;
    }

    this->i2c->iic_control(DW_IIC_CMD_SET_TXCB, (void*)NULL);
    this->i2c->iic_control(DW_IIC_CMD_SET_RXCB, (void*)NULL);
    this->i2c->iic_control(DW_IIC_CMD_SET_ERRCB, (void*)NULL);
    this->i2c->iic_control(DW_IIC_CMD_SET_STACB, (void*)NULL);

    this->i2c = nullptr;

    hx_drv_i2cs_deinit((USE_DW_IIC_SLV_E)USE_DW_IIC_SLV_0);

    this->_is_present = false;
    this->_is_init    = false;
    delete this->rx_ring_buffer;
    delete this->tx_ring_buffer;

    this->rx_ring_buffer = nullptr;
    this->tx_ring_buffer = nullptr;

    return !this->_is_init ? EL_OK : EL_EIO;
}

char WireWE2::echo(bool only_visible) {
    if (!this->_is_present) {
        return EL_EIO;
    }

    return EL_ENOTSUP;
}

char WireWE2::get_char() {
    if (!this->_is_present) {
        return EL_EIO;
    }

    char c{'\0'};

    this->rx_ring_buffer->pop(&c, 1);

    return c;
}

size_t WireWE2::get_line(char* buffer, size_t size, const char delim) {
    if (!this->_is_present) {
        return EL_EIO;
    }

    size_t pos{0};
    char   c{'\0'};
    while (pos < size - 1) {
        if (this->available() == 0) {
            el_sleep(5);
            continue;
        }
        c = this->get_char();
        if (c == delim || c == 0x00) [[unlikely]] {
            buffer[pos++] = '\0';
            return pos;
        }
        buffer[pos++] = c;
    }
    buffer[pos++] = '\0';

    return pos;
}

size_t WireWE2::available() {
    if (!this->_is_present) {
        return EL_EIO;
    }

    return this->rx_ring_buffer->size();
}

el_err_code_t WireWE2::read_bytes(char* buffer, size_t size) {
    if (!this->_is_present) {
        return EL_EIO;
    }

    this->rx_ring_buffer->pop(buffer, size);

    return EL_OK;
}

el_err_code_t WireWE2::send_bytes(const char* buffer, size_t size) {
    if (!this->_is_present) {
        el_printf("not present \n");
        return EL_EIO;
    }
    el_printf("send_bytes %d \n", size);
    this->tx_ring_buffer->push(buffer, size);

    return EL_OK;
}

void WireWE2::set_present(bool is_present) {
    el_printf("set present: %d\n", is_present);
    this->_is_present = is_present;
}

void WireWE2::wire_read_enable(size_t size) {
    memset(this->rx_buffer, 0, sizeof(this->rx_buffer));
    hx_CleanDCache_by_Addr((void*)this->rx_buffer, size);
    IIC_ERR_CODE_E ret =
      hx_drv_i2cs_interrupt_read(this->index, this->addr, this->rx_buffer, size, (void*)i2c_s_callback_fun_rx);
}
void WireWE2::wire_write_enable(size_t size) {
    hx_CleanDCache_by_Addr((void*)this->tx_buffer, size);
    IIC_ERR_CODE_E ret =
      hx_drv_i2cs_interrupt_write(this->index, this->addr, this->tx_buffer, size, (void*)i2c_s_callback_fun_tx);
}

}  // namespace edgelab
