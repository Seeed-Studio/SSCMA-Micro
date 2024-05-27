#ifndef _MA_CV_H_
#define _MA_CV_H_

#include "core/ma_common.h"

namespace ma::cv {

#ifdef __cplusplus
extern "C" {
#endif

ma_err_t convert(const ma_img_t* src, ma_img_t* dst);


#ifdef __cplusplus
}
#endif

}  // namespace ma::cv


#endif  // _MA_CV_H_