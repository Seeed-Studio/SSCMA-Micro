#ifndef _MA_NMS_H_
#define _MA_NMS_H_

#include <algorithm>
#include <iterator>
#include <type_traits>

#include "core/ma_types.h"

namespace ma::utils {

template <typename T, typename = std::void_t<>> struct has_iterator_support : std::false_type {};

template <typename T>
struct has_iterator_support<T, std::void_t<typename T::iterator, typename T::const_iterator>> : std::true_type {};

template <typename T> constexpr bool has_iterator_support_v = has_iterator_support<T>::value;

template <typename Container,
          typename T = typename Container::value_type,
          std::enable_if_t<has_iterator_support_v<Container> && std::is_base_of_v<ma_bbox_t, T>, bool> = true>
 constexpr void nms(
  Container bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target);

}  // namespace ma::utils

#endif  // _MA_NMS_H_