#ifndef _MA_DEVICE_HIMAX_H_
#define _MA_DEVICE_HIMAX_H_

#include "porting/ma_device.h"

namespace ma {

class DeviceHimax : public Device {
public:
    DeviceHimax();
    ~DeviceHimax();

    ma_err_t init() override;
    ma_err_t deinit() override;
};

}  // namespace ma

#endif  // _MA_DEVICE_RECAMERA_H_