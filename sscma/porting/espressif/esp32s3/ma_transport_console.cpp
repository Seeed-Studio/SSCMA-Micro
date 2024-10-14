#include "ma_transport_console.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>

#include <driver/usb_serial_jtag.h>
#include "core/utils/ma_ringbuffer.hpp"


namespace ma {

static auto _driver_config          = usb_serial_jtag_driver_config_t{.tx_buffer_size = 8192, .rx_buffer_size = 8192};
static SPSCRingBuffer<char>* _rb_rx = nullptr;

Console::Console() : Transport(MA_TRANSPORT_CONSOLE) {}

Console::~Console() {
    deInit();
}

ma_err_t Console::init(const void* config) {
    if (m_initialized) {
        return MA_OK;
    }

    (void)config;


    if (usb_serial_jtag_driver_install(&_driver_config) != ESP_OK) {
        return MA_EIO;
    }

    if (!usb_serial_jtag_is_connected()) {
        return MA_EPERM;
    }

    if (_rb_rx == nullptr) {
        _rb_rx = new SPSCRingBuffer<char>(4096);
    }

    m_initialized = true;

    return MA_OK;
}

void Console::deInit() {
    if (!m_initialized) {
        return;
    }

    if (_rb_rx) {
        delete _rb_rx;
        _rb_rx = nullptr;
    }

    usb_serial_jtag_driver_uninstall();

    m_initialized = false;
}

size_t Console::available() const {
    return _rb_rx->size();
}

size_t Console::send(const char* data, size_t length) {
    if (!m_initialized) {
        return 0;
    }

    size_t sent{0};
    size_t pos_of_bytes{0};
    while (length) {
        size_t bytes_to_send{length < _driver_config.tx_buffer_size ? length : _driver_config.tx_buffer_size};

        sent += usb_serial_jtag_write_bytes(data + pos_of_bytes, bytes_to_send, portMAX_DELAY);
        pos_of_bytes += bytes_to_send;
        length -= bytes_to_send;
    }

     // ! https://github.com/espressif/esp-idf/issues/13162
    fsync(fileno(stdout));

    return sent;
}

size_t Console::flush() {
    if (!m_initialized) {
        return 0;
    }

    return  fsync(fileno(stdout));
}

size_t Console::receive(char* data, size_t length) {
    if (!m_initialized) {
        return 0;
    }

    size_t rlen     = 0;
    char   rbuf[32] = {0};  // Most commands are less than 32 bytes long
    do {
        rlen = usb_serial_jtag_read_bytes(rbuf, sizeof(rbuf), 10 / portTICK_PERIOD_MS);
        _rb_rx->push(rbuf, rlen);
    } while (rlen > 0);


    return _rb_rx->pop(data, length);
}

size_t Console::receiveIf(char* data, size_t length, char delimiter) {
    if (!m_initialized) {
        return 0;
    }

    size_t rlen     = 0;
    char   rbuf[32] = {0};  // Most commands are less than 32 bytes long
    do {
        rlen = usb_serial_jtag_read_bytes(rbuf, sizeof(rbuf), 10 / portTICK_PERIOD_MS);
        _rb_rx->push(rbuf, rlen);
    } while (rlen > 0);

    return _rb_rx->popIf(data, length, delimiter);
}


}  // namespace ma
