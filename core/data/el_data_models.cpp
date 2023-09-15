/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 nullptr (Seeed Technology Inc.)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "el_data_models.h"

#include <algorithm>

namespace edgelab {

Models::Models()
    : __partition_start_addr(0u),
      __partition_size(0u),
      __flash_2_memory_map(nullptr),
      __mmap_handler(),
      __model_id_mask(0u),
      __model_info() {}

Models::~Models() { deinit(); }

el_err_code_t Models::init(const char* partition_name, const el_model_format_v& model_format) {
    el_err_code_t ret = el_model_partition_mmap_init(
      partition_name, &__partition_start_addr, &__partition_size, &__flash_2_memory_map, &__mmap_handler);
    if (ret != EL_OK) return ret;
    seek_models_from_flash(model_format);
    return ret;
}

void Models::deinit() {
    el_model_partition_mmap_deinit(&__mmap_handler);
    __flash_2_memory_map = nullptr;
    __mmap_handler       = el_model_mmap_handler_t{};
    __model_info.clear();
}

size_t Models::seek_models_from_flash(const el_model_format_v& model_format) {
    if (!__flash_2_memory_map) [[unlikely]]
        return 0u;

    __model_id_mask = 0u;
    __model_info.clear();

    switch (model_format) {
    case EL_MODEL_FMT_PACKED_TFLITE:
        m_seek_packed_models_from_flash();
        return std::distance(__model_info.begin(), __model_info.end());
    case EL_MODEL_FMT_PLAIN_TFLITE:
        m_seek_plain_models_from_flash();
        return std::distance(__model_info.begin(), __model_info.end());
    case EL_MODEL_FMT_PACKED_TFLITE | EL_MODEL_FMT_PLAIN_TFLITE:
        m_seek_packed_models_from_flash();
        m_seek_plain_models_from_flash();
        return std::distance(__model_info.begin(), __model_info.end());
    default:
        return 0u;
    }
}

void Models::m_seek_packed_models_from_flash() {
    const uint8_t*           mem_addr = nullptr;
    const el_model_header_t* header   = nullptr;
    for (size_t it = 0u; it < __partition_size; it += sizeof(el_model_header_t)) {
        mem_addr = __flash_2_memory_map + it;
        header   = reinterpret_cast<const el_model_header_t*>(mem_addr);
        if ((el_ntohl(header->b4[0]) & 0xFFFFFF00) != (CONFIG_EL_MODEL_HEADER_MAGIC << 8u)) continue;

        uint8_t  model_id   = header->b1[3] >> 4u;
        uint8_t  model_type = header->b1[3] & 0x0F;
        uint32_t model_size = (el_ntohl(header->b4[1]) & 0xFFFFFF00) >> 8u;
        if (!model_id || !model_type || !model_size || model_size > (__partition_size - it)) [[unlikely]]
            continue;

        if (~__model_id_mask & (1u << model_id)) {
            __model_info.emplace_front(el_model_info_t{.id          = model_id,
                                                       .type        = static_cast<el_algorithm_type_t>(model_type),
                                                       .addr_flash  = __partition_start_addr + it,
                                                       .size        = model_size,
                                                       .addr_memory = mem_addr + sizeof(el_model_header_t)});
            __model_id_mask |= (1u << model_id);
        }
        it += model_size;
    }
}

void Models::m_seek_plain_models_from_flash() {
    const uint8_t*           mem_addr = nullptr;
    const el_model_header_t* header   = nullptr;
    uint8_t                  model_id = 1u;
    for (size_t it = 0u; it < __partition_size; it += sizeof(el_model_header_t)) {
        mem_addr = __flash_2_memory_map + it;
        header   = reinterpret_cast<const el_model_header_t*>(mem_addr);
        if (el_ntohl(header->b4[1]) != CONFIG_EL_MODEL_TFLITE_MAGIC) [[likely]]
            continue;

        if (std::find_if(__model_info.begin(), __model_info.end(), [&](const el_model_info_t& v) {
                return v.addr_memory == mem_addr;
            }) != __model_info.end())
            break;

        while (__model_id_mask & (1u << model_id))
            if (++model_id >= (sizeof(__model_id_mask) << 3u)) [[unlikely]]
                return;

        __model_info.emplace_front(el_model_info_t{.id          = model_id,
                                                   .type        = EL_ALGO_TYPE_UNDEFINED,
                                                   .addr_flash  = __partition_start_addr + it,
                                                   .size        = 0u,
                                                   .addr_memory = mem_addr});
        __model_id_mask |= (1u << model_id);
    }
}

bool Models::has_model(el_model_id_t model_id) const { return __model_id_mask & (1u << model_id); }

el_err_code_t Models::get(el_model_id_t model_id, el_model_info_t& model_info) const {
    auto it = std::find_if(
      __model_info.begin(), __model_info.end(), [&](const el_model_info_t& v) { return v.id == model_id; });
    if (it != __model_info.end()) [[likely]] {
        model_info = *it;
        return EL_OK;
    }
    return EL_EINVAL;
}

el_model_info_t Models::get_model_info(el_model_id_t model_id) const {
    auto it = std::find_if(
      __model_info.begin(), __model_info.end(), [&](const el_model_info_t& v) { return v.id == model_id; });
    if (it != __model_info.end()) [[likely]] {
        return *it;
    }
    return {};
}

const std::forward_list<el_model_info_t>& Models::get_all_model_info() const { return __model_info; }

size_t Models::get_all_model_info_size() const { return std::distance(__model_info.begin(), __model_info.end()); }

}  // namespace edgelab
