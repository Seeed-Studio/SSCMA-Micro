#ifndef _MA_TRANSPORT_H_
#define _MA_TRANSPORT_H_

#include <core/ma_types.h>
#include <ma_config_board.h>

#include <cstddef>
#include <cstdint>

namespace ma {

class Transport {
   public:
    explicit Transport(ma_transport_type_t type) noexcept : m_initialized(false), m_type(type) {}
    virtual ~Transport() = default;

    Transport(const Transport&)            = delete;
    Transport& operator=(const Transport&) = delete;

    [[nodiscard]] virtual ma_err_t init(const void* config) noexcept = 0;
    virtual void                   deInit() noexcept                 = 0;

    [[nodiscard]] operator bool() const noexcept { return m_initialized; }
    [[nodiscard]] ma_transport_type_t getType() const noexcept { return m_type; }

    [[nodiscard]] virtual size_t available() const noexcept                                    = 0;
    virtual size_t               send(const char* data, size_t length) noexcept                = 0;
    virtual size_t               flush() noexcept                                              = 0;
    virtual size_t               receive(char* data, size_t length) noexcept                   = 0;
    virtual size_t               receiveIf(char* data, size_t length, char delimiter) noexcept = 0;

   protected:
    bool                m_initialized;
    ma_transport_type_t m_type;
};

}  // namespace ma

#endif  // _MA_TRANSPORT_H_