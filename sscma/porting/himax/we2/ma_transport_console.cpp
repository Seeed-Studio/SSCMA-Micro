

#include "ma_transport_console.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>

extern "C" {
#include <console_io.h>
#include <hx_drv_uart.h>
}

#include <porting/ma_osal.h>

#include <core/utils/ma_ringbuffer.hpp>

#include "ma_config_board.h"

namespace ma {

static SPSCRingBuffer<char>* _rb_rx   = nullptr;
static SPSCRingBuffer<char>* _rb_tx   = nullptr;
static char*                 _rx_buf  = nullptr;
static char*                 _tx_buf  = nullptr;
static volatile bool         _tx_busy = false;
static Mutex                 _tx_mutex;
static DEV_UART*             _uart      = nullptr;
static volatile bool         _is_opened = false;

static void _uart_dma_recv(void*) {
    if (!_is_opened || !_rb_rx || !_rx_buf || !_uart) {
        return;
    }
    _rb_rx->push(_rx_buf, 1);
    _uart->uart_read_udma(_rx_buf, 1, reinterpret_cast<void*>(_uart_dma_recv));
}

static void _uart_dma_send(void*) {
    if (!_is_opened || !_rb_tx || !_tx_buf || !_uart) {
        _tx_busy = false;
        return;
    }
    size_t remain = std::min(_rb_tx->size(), static_cast<size_t>(4095));
    if ((_tx_busy = (remain != 0))) {
        remain = _rb_tx->pop(_tx_buf, remain);
        SCB_CleanDCache_by_Addr(_tx_buf, remain);
        _uart->uart_write_udma(_tx_buf, remain, reinterpret_cast<void*>(_uart_dma_send));
    }
}

Console::Console() : Transport(MA_TRANSPORT_CONSOLE) {}

Console::~Console() { deInit(); }

ma_err_t Console::init(const void* config) {
    if (_is_opened || m_initialized) {
        return MA_OK;
    }

    (void)config;

    _uart = hx_drv_uart_get_dev(USE_DW_UART_0);
    if (_uart == nullptr) {
        return MA_EIO;
    }

    int ret = _uart->uart_open(UART_BAUDRATE_921600);
    if (ret != 0) {
        return MA_EIO;
    }

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

    _uart->uart_read_udma(_rx_buf, 1, reinterpret_cast<void*>(_uart_dma_recv));

    return MA_OK;
}

void Console::deInit() {
    if (!_is_opened || !m_initialized) {
        return;
    }

    while (_tx_busy) {
        ma::Thread::yield();
    }

    if (_uart) {
        _uart->uart_close();
        hx_drv_uart_deinit(USE_DW_UART_0);
        _uart = nullptr;
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

size_t Console::available() const { return _rb_rx->size(); }

size_t Console::send(const char* data, size_t length) {
    if (!m_initialized || data == nullptr || length == 0) {
        return 0;
    }

    Guard guard(_tx_mutex);

    size_t bytes_to_send = 0;
    size_t sent          = 0;

    while (length) {
        bytes_to_send = _rb_tx->push(data + sent, length);
        length -= bytes_to_send;
        sent += bytes_to_send;

        if (!_tx_busy) {
            _tx_busy      = true;
            bytes_to_send = std::min(_rb_tx->size(), static_cast<size_t>(4095));
            _rb_tx->pop(_tx_buf, bytes_to_send);
            SCB_CleanDCache_by_Addr(_tx_buf, bytes_to_send);
            _uart->uart_write_udma(_tx_buf, bytes_to_send, reinterpret_cast<void*>(_uart_dma_send));
        } else {
            ma::Thread::yield();
        }
    }

    return sent;
}

size_t Console::flush() {
    if (!m_initialized) {
        return -1;
    }

    return 0;
}

size_t Console::receive(char* data, size_t length) {
    if (!m_initialized || length == 0) {
        return 0;
    }

    return _rb_rx->pop(data, length);
}

size_t Console::receiveIf(char* data, size_t length, char delimiter) {
    if (!m_initialized || length == 0) {
        return 0;
    }

    return _rb_rx->popIf(data, length, delimiter);
}

}  // namespace ma
