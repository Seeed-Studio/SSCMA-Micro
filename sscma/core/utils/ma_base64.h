#ifndef _MA_BASE64_H_
#define _MA_BASE64_H_
#include "../ma_types.h"


#include <cstdint>
#include <string>

namespace ma::utils {

ma_err_t base64_encode(const unsigned char* in, int in_len, char* out, int* out_len);

std::string base64_decode(const std::string &in);

}  // namespace ma::utils

#endif