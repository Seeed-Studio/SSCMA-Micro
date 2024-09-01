#ifndef _XIP_H_
#define _XIP_H_

#ifdef __cplusplus
extern "C" {
#endif


extern void xip_ownership_acquire();
extern void xip_ownership_release();

extern bool xip_safe_enable();
extern bool xip_safe_disable();

#ifdef __cplusplus
}
#endif

#endif  // _XIP_H_