#pragma once

#include <algorithm>
#include <cstdint>
#include <forward_list>
#include <list>
#include <utility>

#include "core/el_types.h"
#include "sscma/utility.hpp"

namespace sscma::utility {

// categorize results by target class
// the returned view is a list of pairs of target class and a list of pointers to results
// so the safety of the view is dependent on the lifetime of the input list
template <typename ResultType>
inline decltype(auto) get_cls_view_from_results(std::forward_list<ResultType> const& res) {
    // transform results to pointers view of classes
    // index view would be safer but std::forward_list doesn't support random access by index efficiently
    std::forward_list<std::pair<decltype(ResultType::target), std::forward_list<const ResultType*>>> cls_view;
    for (auto const& i : res) {
        // if target class is not in the view, add it
        auto it = std::find_if(cls_view.begin(), cls_view.end(), [&i](auto const& e) { return e.first == i.target; });
        if (it == cls_view.end())
            cls_view.push_front({i.target, {&i}});
        else  // else, add the result to the class view
            it->second.push_front(&i);
    }
    return cls_view;
}

// compare boxes using a euclidean distance like metric
// we caculate the distance of two boxes by the sum of the absolute value of the difference of each dimension
// we divide the score difference by 4 to make it less important in the distance calculation
inline bool compare_result_pair(el_box_t const* l, el_box_t const* r) {
    auto dx       = l->x - r->x;
    auto dy       = l->y - r->y;
    auto dw       = l->w - r->w;
    auto dh       = l->h - r->h;
    auto ds       = l->score - r->score;
    auto distance = std::abs(dx) + std::abs(dy) + std::abs(dw) + std::abs(dh) + (std::abs(ds) >> 2);  // kscore = 1/4
    return distance > 5;                                                                              // threshold is 5
}

// compare points using a euclidean distance like metric
// we caculate the distance of two points by the sum of the absolute value of the difference of each dimension
// we divide the score difference by 4 to make it less important in the distance calculation
inline bool compare_result_pair(el_point_t const* l, el_point_t const* r) {
    auto dx       = l->x - r->x;
    auto dy       = l->y - r->y;
    auto ds       = l->score - r->score;
    auto distance = std::abs(dx) + std::abs(dy) + (std::abs(ds) >> 2);  // kscore = 1/4
    return distance > 5;                                                // threshold is 5
}
// compare classes using a euclidean distance like metric
// we caculate the distance of two classes by the absolute value of the difference of each dimension
// we divide the score difference by 4 to make it less important in the distance calculation
inline bool compare_result_pair(el_class_t const* l, el_class_t const* r) {
    auto ds       = l->score - r->score;
    auto distance = std::abs(ds) >> 2;  // kscore = 1/4
    return distance > 5;                // threshold is 5
}

// compare results using a euclidean distance like metric
template <typename ResultType> class ResultsFilter {
   public:
    // initialize with a list of results (copy when calling the constructor, its view should bind with itself)
    ResultsFilter(std::forward_list<ResultType> init) : _last(init), _last_cls_view(get_cls_view_from_results(_last)) {}

    ~ResultsFilter() = default;

    // compare the input results with the last results (copy on call)
    bool compare_and_update(std::forward_list<ResultType> current) {
        // transform results to pointers view of classes (move semantics is required when updating last results)
        auto current_cls_view{get_cls_view_from_results(current)};

        // if the number of classes is different, the results are different
        if (std::distance(_last.begin(), _last.end()) != std::distance(current.begin(), current.end()))
            goto ResultDifferent;

        // if the number of classes is different, the results are different
        if (std::distance(_last_cls_view.begin(), _last_cls_view.end()) !=
            std::distance(current_cls_view.begin(), current_cls_view.end()))
            goto ResultDifferent;

        // compare each class
        for (const auto& [cls, objs] : _last_cls_view) {
            // if the class is not in the current results, the results are different
            auto sub_view = std::find_if(
              current_cls_view.begin(), current_cls_view.end(), [&cls](auto const& e) { return e.first == cls; });
            if (sub_view == current_cls_view.end()) goto ResultDifferent;

            // if the number of objects is different, the results are different
            if (std::distance(objs.begin(), objs.end()) !=
                std::distance(sub_view->second.begin(), sub_view->second.end()))
                goto ResultDifferent;

            // create a temporary list for linear removal complexity
            auto sub_view_objs = std::list(sub_view->second.cbegin(), sub_view->second.cend());
            for (auto const& i : objs) {
                // if the object is not in the current results, the results are different
                auto compare_objs = std::find_if(sub_view_objs.begin(), sub_view_objs.end(), [&i](auto const& e) {
                    return compare_result_pair(e, i);
                });
                if (compare_objs == sub_view_objs.end())
                    goto ResultDifferent;
                else  // else, remove the object from the temporary list
                    sub_view_objs.erase(compare_objs);
            }
        }

        // if the results are the same, return false
        // move is required to update the last results (because the elements in the view are pointers of the results)
        _last          = std::move(current);
        _last_cls_view = std::move(current_cls_view);
        return false;

    ResultDifferent:
        // if the results are different, return true
        // move is required to update the last results (because the elements in the view are pointers of the results)
        _last          = std::move(current);
        _last_cls_view = std::move(current_cls_view);
        return true;
    }

   private:
    std::forward_list<ResultType>                                                                    _last;
    std::forward_list<std::pair<decltype(ResultType::target), std::forward_list<const ResultType*>>> _last_cls_view;
};

}  // namespace sscma::utility
