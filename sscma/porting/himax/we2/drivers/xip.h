#ifndef _XIP_H_
#define _XIP_H_

#ifdef __cplusplus
extern "C" {
#endif

bool xip_drv_init();

void xip_ownership_acquire();
void xip_ownership_release();

bool xip_safe_enable();
bool xip_safe_disable();

#ifdef __cplusplus
}
#endif

#endif  // _XIP_H_