#ifndef _NPU_H_
#define _NPU_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int himax_arm_npu_init(bool security_enable, bool privilege_enable);

#ifdef __cplusplus
}
#endif

#endif