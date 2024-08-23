#include "ma_base64.h"
#include <cstdio>

namespace ma::utils {

static const char* BASE64_CHARS_TABLE =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

ma_err_t base64_encode(const unsigned char* in, int in_len, char* out, int* out_len) {
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    if (*out_len < 4 * ((in_len + 2) / 3)) {
        *out_len = 4 * ((in_len + 2) / 3);
        return MA_OVERFLOW;
    }

    *out_len = 4 * ((in_len + 2) / 3);

    while (in_len--) {
        char_array_3[i++] = *(in++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                *out++ = BASE64_CHARS_TABLE[char_array_4[i]];

            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) {
            *out++ = BASE64_CHARS_TABLE[char_array_4[j]];
        }

        while (i++ < 3)
            *out++ = '=';
    }
    return MA_OK;
}

}  // namespace ma::utils