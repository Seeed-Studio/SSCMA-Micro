#ifndef _MA_TRANSPORT_CONSOLE_H_
#define _MA_TRANSPORT_CONSOLE_H_

#include "porting/ma_transport.h"

namespace ma {

class Console final : public Transport {
   public:
    Console();
    ~Console();

    ma_err_t init(void* config) override;
    ma_err_t deInit() override;

    size_t available() const override;
    size_t send(const char* data, size_t length, int timeout = -1) override;
    size_t flush() override;
    size_t receive(char* data, size_t length, int timeout = 1) override;
    size_t receiveUntil(char* data, size_t length, char delimiter, int timeout = 1) override;
};

}  // namespace ma

#endif  // _MA_CONSOLE_H_