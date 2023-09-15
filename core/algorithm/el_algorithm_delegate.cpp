/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
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

#include "el_algorithm_delegate.h"

#include <algorithm>

#include "core/el_types.h"

namespace edgelab {

namespace utility {

// Note: the order index influences the algorthm type in current implementation
el_algorithm_type_t el_algorithm_type_from_engine(const Engine* engine) {
#ifdef _EL_ALGORITHM_YOLO_H_  // index 1
    if (AlgorithmYOLO::is_model_valid(engine)) return EL_ALGO_TYPE_YOLO;
#endif
#ifdef _EL_ALGORITHM_FOMO_H_  // index 2
    if (AlgorithmFOMO::is_model_valid(engine)) return EL_ALGO_TYPE_FOMO;
#endif
#ifdef _EL_ALGORITHM_IMCLS_H_  // index 3
    if (AlgorithmIMCLS::is_model_valid(engine)) return EL_ALGO_TYPE_IMCLS;
#endif
#ifdef _EL_ALGORITHM_PFLD_H_  // index 4
    if (AlgorithmPFLD::is_model_valid(engine)) return EL_ALGO_TYPE_PFLD;
#endif
    return EL_ALGO_TYPE_UNDEFINED;
}

}  // namespace utility

AlgorithmDelegate* AlgorithmDelegate::get_delegate() {
    static AlgorithmDelegate data_delegate = AlgorithmDelegate();
    return &data_delegate;
}

AlgorithmDelegate::InfoType AlgorithmDelegate::get_algorithm_info(el_algorithm_type_t type) const {
    auto it = std::find_if(
      _registered_algorithms.begin(), _registered_algorithms.end(), [&](const InfoType* i) { return i->type == type; });
    if (it != _registered_algorithms.end()) return **it;
    return {};
}

const std::forward_list<const AlgorithmDelegate::InfoType*>& AlgorithmDelegate::get_all_algorithm_info() const {
    return _registered_algorithms;
}

size_t AlgorithmDelegate::get_all_algorithm_info_count() const {
    return std::distance(_registered_algorithms.begin(), _registered_algorithms.end());
}

bool AlgorithmDelegate::has_algorithm(el_algorithm_type_t type) const {
    auto it = std::find_if(
      _registered_algorithms.begin(), _registered_algorithms.end(), [&](const InfoType* i) { return i->type == type; });
    return it != _registered_algorithms.end();
}

AlgorithmDelegate::AlgorithmDelegate() {
#ifdef _EL_ALGORITHM_FOMO_H_
    _registered_algorithms.emplace_front(&AlgorithmFOMO::algorithm_info);
#endif
#ifdef _EL_ALGORITHM_PFLD_H_
    _registered_algorithms.emplace_front(&AlgorithmPFLD::algorithm_info);
#endif
#ifdef _EL_ALGORITHM_YOLO_H_
    _registered_algorithms.emplace_front(&AlgorithmYOLO::algorithm_info);
#endif
#ifdef _EL_ALGORITHM_IMCLS_H_
    _registered_algorithms.emplace_front(&AlgorithmIMCLS::algorithm_info);
#endif
}

}  // namespace edgelab
