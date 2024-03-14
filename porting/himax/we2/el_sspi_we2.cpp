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

#include "el_sspi_we2.h"

#include <cctype>
#include <cstdint>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "el_config_porting.h"

namespace edgelab {

namespace porting {

static sspiWE2* sspi_ptr = nullptr;

static void sspi_txcb(void* param) {
    // el_printf("%s \n", __FUNCTION__);
    // el_printf("tx\n");
    // for (uint32_t i = 0; i < 16; i++) {
    //     el_printf("0x%02x%s", sspi_ptr->tx_buffer[i], ((i + 1) % 16) ? " " : "\n");
    // }
    memset(sspi_ptr->tx_buffer, 0, sizeof(sspi_ptr->tx_buffer));
    if (sspi_ptr->tx_ring_buffer->isEmpty()) {
        hx_drv_gpio_set_output(CONFIG_EL_SPI_CTRL_PIN, GPIO_OUT_LOW);
    }
    sspi_ptr->sspi_read_enable(sizeof(sspi_ptr->rx_buffer));
}

static void sspi_rxcb(void* param) {
    uint8_t  feature = sspi_ptr->rx_buffer[0];
    uint8_t  cmd     = sspi_ptr->rx_buffer[1];
    uint16_t len     = sspi_ptr->rx_buffer[2] << 8 | sspi_ptr->rx_buffer[3];
    // uint16_t crc  = sspi_ptr->rx_buffer[4 + len] << 8 | sspi_ptr->rx_buffer[5 + len];

    // for (uint32_t i = 0; i < 256; i++) {
    //     el_printf("0x%02x%s", sspi_ptr->rx_buffer[i], ((i + 1) % 16) ? " " : "\n");
    // }

    if (len > SPI_WRITE_PL_LEN || feature != FEATURE_TRANSPORT) {
        EL_LOGW("Invalid data received, feature: %d, cmd: %d, len: %d\n", feature, cmd, len);
        sspi_ptr->sspi_read_enable(sizeof(sspi_ptr->rx_buffer));
        return;
    }
    // el_printf("f: %d, c: %d, l: %d\n", feature, cmd, len);
    switch (cmd) {
    case FEATURE_TRANSPORT_CMD_WRITE:
        sspi_ptr->rx_ring_buffer->put((char*)&sspi_ptr->rx_buffer[4], len);
        sspi_ptr->sspi_read_enable(sizeof(sspi_ptr->rx_buffer));
        break;
    case FEATURE_TRANSPORT_CMD_READ:
        if (len > sspi_ptr->tx_ring_buffer->size()) {
            len = sspi_ptr->tx_ring_buffer->size();
        }
        // el_printf("read: %d\n", len);
        sspi_ptr->tx_ring_buffer->get((char*)&sspi_ptr->tx_buffer[0], len);
        sspi_ptr->sspi_write_enable(len);
        break;
    case FEATURE_TRANSPORT_CMD_AVAILABLE:
        sspi_ptr->tx_buffer[0] = (sspi_ptr->tx_ring_buffer->size() >> 8) & 0xff;
        sspi_ptr->tx_buffer[1] = sspi_ptr->tx_ring_buffer->size() & 0xff;
        sspi_ptr->sspi_write_enable(2);
        break;
    case FEATURE_TRANSPORT_CMD_RESET:
        sspi_ptr->rx_ring_buffer->clear();
        sspi_ptr->tx_ring_buffer->clear();
        sspi_ptr->sspi_read_enable(sizeof(sspi_ptr->rx_buffer));
        hx_drv_gpio_set_output(CONFIG_EL_SPI_CTRL_PIN, GPIO_OUT_LOW);
        break;
    default:
        sspi_ptr->sspi_read_enable(sizeof(sspi_ptr->rx_buffer));
        break;
    }
}

}  // namespace porting

using namespace porting;

sspiWE2::sspiWE2() {}

sspiWE2::~sspiWE2() { deinit(); }

el_err_code_t sspiWE2::init() {
    EL_ASSERT(!this->_is_present);

    CONFIG_EL_SPI_CTRL_INIT_F;
    hx_drv_gpio_set_output(CONFIG_EL_SPI_CTRL_PIN, GPIO_OUT_LOW);

    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_S_DO, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_S_DI, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_S_CLK, 1);
    // hx_drv_scu_set_PB11_pinmux(SCU_PB11_PINMUX_SPI_S_CS, 1);
    CONFIG_EL_SPI_CS_INIT_F;

    hx_drv_spi_slv_init(USE_DW_SPI_SLV_0, DW_SSPI_S_RELBASE);
    this->spi = hx_drv_spi_slv_get_dev(USE_DW_SPI_SLV_0);
    this->spi->spi_open(DEV_SLAVE_MODE, SPI_CPOL_0_CPHA_0);

    this->_is_present = this->spi ? true : false;

    this->tx_ring_buffer = new lwRingBuffer(SPI_WRITE_PL_LEN * 16);
    this->rx_ring_buffer = new lwRingBuffer(SPI_READ_PL_LEN * 4);

    this->sspi_read_enable(sizeof(this->rx_buffer));

    sspi_ptr = this;

    return this->_is_present ? EL_OK : EL_EIO;
}

el_err_code_t sspiWE2::deinit() {
    if (this->spi == nullptr) {
        return EL_OK;
    }

    this->spi->spi_control(SPI_CMD_RESET, (void*)NULL);
    this->spi = nullptr;

    this->_is_present = false;
    delete this->rx_ring_buffer;
    delete this->tx_ring_buffer;

    this->rx_ring_buffer = nullptr;
    this->tx_ring_buffer = nullptr;

    return EL_OK;
}

char sspiWE2::echo(bool only_visible) { return 0; }

char sspiWE2::get_char() {
    if (!this->_is_present) {
        return 0;
    }
    return this->rx_ring_buffer->get();
}

size_t sspiWE2::get_line(char* buffer, size_t size, const char delim) {
    if (!this->_is_present) {
        return 0;
    }
    return this->rx_ring_buffer->extract(delim, buffer, size);
}

size_t sspiWE2::available() {
    if (!this->_is_present) {
        return 0;
    }
    return this->rx_ring_buffer->size();
}

size_t sspiWE2::read_bytes(char* buffer, size_t size) {
    if (!this->_is_present) {
        return 0;
    }
    return this->rx_ring_buffer->get(buffer, size);
}

size_t sspiWE2::send_bytes(const char* buffer, size_t size) {
    if (!this->_is_present) {
        return 0;
    }
    el_printf("buf %d, put %d\n", this->tx_ring_buffer->size(), size);
    // if (this->tx_ring_buffer->free() < size) {
    //     EL_LOGW("TX ring buffer full, cannot send data\n");
    //     return 0;
    // }
    uint32_t ts = xTaskGetTickCount();
    while (this->tx_ring_buffer->free() < size) {
        if (xTaskGetTickCount() - ts > 1000) {
            EL_LOGW("TX ring buffer full, cannot send data\n");
            this->tx_ring_buffer->clear();
            hx_drv_gpio_set_output(CONFIG_EL_SPI_CTRL_PIN, GPIO_OUT_LOW);
            return 0;
        }
        el_sleep(5);
    }
    size_t len = this->tx_ring_buffer->put(buffer, size);
    hx_drv_gpio_set_output(CONFIG_EL_SPI_CTRL_PIN, GPIO_OUT_HIGH);
    return len;
}

void sspiWE2::sspi_read_enable(size_t size) {
    // el_printf("re: %d\n", size);
    memset(this->rx_buffer, 0, sizeof(this->rx_buffer));
    hx_CleanDCache_by_Addr((void*)this->rx_buffer, size);
    this->spi->spi_read_dma(this->rx_buffer, size, (void*)sspi_rxcb);
}

void sspiWE2::sspi_write_enable(size_t size) {
    // el_printf("we: %d\n", size);
    // memset(this->tx_buffer, 0, sizeof(this->tx_buffer));
    hx_CleanDCache_by_Addr((void*)this->tx_buffer, size);
    this->spi->spi_write_dma(this->tx_buffer, size, (void*)sspi_txcb);
}

}  // namespace edgelab
