#ifndef _DRV_COMMON_H_
#define _DRV_COMMON_H_

#include <core/el_debug.h>
#include <core/el_types.h>
#include <inttypes.h>

el_res_t _drv_fit_res(uint16_t width, uint16_t height) {
    el_res_t res;

    if (width > 320 || height > 240) {
        res.width  = 640;
        res.height = 480;
    } else if (width > 160 || height > 120) {
        res.width  = 320;
        res.height = 240;
    } else {
        res.width  = 160;
        res.height = 120;
    }

    EL_LOGD("fit width: %d height: %d", res.width, res.height);

    return res;
}

#endif
