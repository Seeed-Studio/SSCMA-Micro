#ifndef _MA_DEVICE_RECAMERA_H_
#define _MA_DEVICE_RECAMERA_H_

#include "porting/ma_device.h"

namespace ma {

class DevicePosix : public Device {
public:
    DevicePosix();
    ~DevicePosix();

    ma_err_t init() override;
    ma_err_t deinit() override;
};

}  // namespace ma

#endif  // _MA_DEVICE_RECAMERA_H_