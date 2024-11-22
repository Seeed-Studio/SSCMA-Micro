#ifndef _MA_CV_H_
#define _MA_CV_H_

#include "../ma_common.h"

namespace ma::cv {

#ifdef __cplusplus
extern "C" {
#endif

ma_err_t convert(const ma_img_t* src, ma_img_t* dst);

#if MA_USE_LIB_JPEGENC
ma_err_t rgb_to_jpeg(const ma_img_t* src, ma_img_t* dst);
#endif

#ifdef __cplusplus
}
#endif

}  // namespace ma::cv


#endif  // _MA_CV_H_