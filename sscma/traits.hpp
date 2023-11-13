#pragma once

#include <type_traits>

namespace sscma::traits {

// check if a type has a member function named set_score_threshold
template <typename T, typename = void> struct has_method_set_score_threshold : std::false_type {};

template <typename T>
struct has_method_set_score_threshold<
  T,
  typename std::enable_if<std::is_member_function_pointer<decltype(&T::set_score_threshold)>::value>::type>
    : std::true_type {};

// check if a type has a member function named get_score_threshold
template <typename T, typename = void> struct has_method_get_score_threshold : std::false_type {};

template <typename T>
struct has_method_get_score_threshold<
  T,
  typename std::enable_if<std::is_member_function_pointer<decltype(&T::get_score_threshold)>::value>::type>
    : std::true_type {};

// check if a type has a member function named set_iou_threshold
template <typename T, typename = void> struct has_method_set_iou_threshold : std::false_type {};

template <typename T>
struct has_method_set_iou_threshold<
  T,
  typename std::enable_if<std::is_member_function_pointer<decltype(&T::set_iou_threshold)>::value>::type>
    : std::true_type {};

// check if a type has a member function named get_iou_threshold
template <typename T, typename = void> struct has_method_get_iou_threshold : std::false_type {};

template <typename T>
struct has_method_get_iou_threshold<
  T,
  typename std::enable_if<std::is_member_function_pointer<decltype(&T::get_iou_threshold)>::value>::type>
    : std::true_type {};

// check if a type has a member named score_threshold
template <typename T, class = void> struct has_member_score_threshold : std::false_type {};

template <typename T>
struct has_member_score_threshold<T, std::void_t<decltype(T::score_threshold)>> : std::true_type {};

// check if a type has a member named iou_threshold
template <typename T, class = void> struct has_member_iou_threshold : std::false_type {};

template <typename T> struct has_member_iou_threshold<T, std::void_t<decltype(T::iou_threshold)>> : std::true_type {};

}  // namespace sscma::traits
