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

#ifndef _EL_DATA_MODELS_H_
#define _EL_DATA_MODELS_H_

#include <cstdint>
#include <forward_list>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "porting/el_flash.h"

namespace edgelab {

class Models {
   public:
    Models();
    ~Models();

    Models(const Models&)            = delete;
    Models& operator=(const Models&) = delete;

    el_err_code_t init(const char*              partition_name = CONFIG_EL_MODEL_PARTITION_NAME,
                       const el_model_format_v& model_format = EL_MODEL_FMT_PACKED_TFLITE | EL_MODEL_FMT_PLAIN_TFLITE);
    void          deinit();

    size_t                                    seek_models_from_flash(const el_model_format_v& model_format);
    bool                                      has_model(el_model_id_t model_id) const;
    el_err_code_t                             get(el_model_id_t model_id, el_model_info_t& model_info) const;
    el_model_info_t                           get_model_info(el_model_id_t model_id) const;
    const std::forward_list<el_model_info_t>& get_all_model_info() const;
    size_t                                    get_all_model_info_size() const;

   protected:
    void m_seek_packed_models_from_flash();
    void m_seek_plain_models_from_flash();

   private:
    uint32_t                           __partition_start_addr;
    uint32_t                           __partition_size;
    const uint8_t*                     __flash_2_memory_map;
    el_model_mmap_handler_t            __mmap_handler;
    uint16_t                           __model_id_mask;
    std::forward_list<el_model_info_t> __model_info;
};

}  // namespace edgelab::data

#endif
