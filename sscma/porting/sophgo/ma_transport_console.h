#ifndef _MA_CONSOLE_H_
#define _MA_CONSOLE_H_

#include "porting/ma_transport.h"

namespace ma {

class Console final : public Transport {
public:
    Console();
    ~Console();

    ma_err_t open();
    ma_err_t close();

    operator bool() const override;

    size_t available() const override;
    size_t send(const char* data, size_t length, int timeout = -1) override;
    size_t receive(char* data, size_t length, int timeout = 1) override;
};

}  // namespace ma


#endif  // _MA_CONSOLE_H_