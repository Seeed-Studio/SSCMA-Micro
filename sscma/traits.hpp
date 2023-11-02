#pragma once

#include <type_traits>

namespace sscma::traits {

template <typename T, typename = void> struct has_method_set_score_threshold : std::false_type {};

template <typename T>
struct has_method_set_score_threshold<
  T,
  typename std::enable_if<std::is_member_function_pointer<decltype(&T::set_score_threshold)>::value>::type>
    : std::true_type {};

template <typename T, typename = void> struct has_method_get_score_threshold : std::false_type {};

template <typename T>
struct has_method_get_score_threshold<
  T,
  typename std::enable_if<std::is_member_function_pointer<decltype(&T::get_score_threshold)>::value>::type>
    : std::true_type {};

template <typename T, typename = void> struct has_method_set_iou_threshold : std::false_type {};

template <typename T>
struct has_method_set_iou_threshold<
  T,
  typename std::enable_if<std::is_member_function_pointer<decltype(&T::set_iou_threshold)>::value>::type>
    : std::true_type {};

template <typename T, typename = void> struct has_method_get_iou_threshold : std::false_type {};

template <typename T>
struct has_method_get_iou_threshold<
  T,
  typename std::enable_if<std::is_member_function_pointer<decltype(&T::get_iou_threshold)>::value>::type>
    : std::true_type {};

template <typename T, class = void> struct has_member_score_threshold : std::false_type {};

template <typename T>
struct has_member_score_threshold<T, std::void_t<decltype(T::score_threshold)>> : std::true_type {};

template <typename T, class = void> struct has_member_iou_threshold : std::false_type {};

template <typename T> struct has_member_iou_threshold<T, std::void_t<decltype(T::iou_threshold)>> : std::true_type {};

}  // namespace sscma::traits
