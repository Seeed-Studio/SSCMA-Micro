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

#ifndef _EL_WIRE_WE2_H_
#define _EL_WIRE_WE2_H_

extern "C" {
#include <WE2_core.h>
#include <hx_drv_gpio.h>
#include <hx_drv_iic.h>
#include <hx_drv_scu.h>
}

#include "core/utils/el_ringbuffer.hpp"
#include "porting/el_wire.h"

/* I2C Communication Protocol : Format / Field Definition
 * [Feature] [CMD] [Payload Len] [Payload] [checksum]
 * [Feature] - one byte
 * [CMD]     - one byte
 * [PL_LEN ] - two bytes
 * [Payload] - 256bytes (MAX)
 * [checksum]- two bytes (ccitt-crc16)
 */

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

namespace edgelab {

class WireWE2 final : public Wire {
   public:
    WireWE2(uint8_t addr);
    ~WireWE2();

    el_err_code_t init() override;
    el_err_code_t deinit() override;

    char   echo(bool only_visible = true) override;
    char   get_char() override;
    size_t get_line(char* buffer, size_t size, const char delim = 0x0d) override;

    el_err_code_t read_bytes(char* buffer, size_t size) override;
    el_err_code_t send_bytes(const char* buffer, size_t size) override;

    size_t available() override;

   public:
    void wire_read_enable(size_t size);
    void wire_write_enable(size_t size);
    void set_present(bool is_present);

   public:
    lwRingBuffer*    rx_ring_buffer;
    lwRingBuffer*    tx_ring_buffer;
    uint8_t          rx_buffer[MAX_PAYLOAD_LEN];
    uint8_t          tx_buffer[MAX_PAYLOAD_LEN];
    USE_DW_IIC_SLV_E index;
    HX_DRV_DEV_IIC*  i2c;
    bool             _is_init;
};

}  // namespace edgelab

#endif
