#ifndef _MA_BASE64_H_
#define _MA_BASE64_H_
#include "core/ma_types.h"


#include <cstdint>

namespace ma::utils {

ma_err_t base64_encode(const unsigned char* in, int in_len, char* out, int* out_len);

}  // namespace ma::utils

#endif