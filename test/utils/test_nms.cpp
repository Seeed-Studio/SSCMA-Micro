#include <gtest/gtest.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "3rdparty/npy.hpp"

#include "core/ma_types.h"
#include "core/utils/ma_nms.h"

namespace ma {

TEST(UTILS, NMS) {
    std::string current_path = __FILE__;
    current_path             = current_path.substr(0, current_path.find_last_of("/"));

    std::cout << "current_path: " << current_path << std::endl;
    std::string npy_dir = current_path + "/test_data";
    int i               = 0;

    std::string raw_file  = npy_dir + "/random_bboxes.npy";
    npy::npy_data raw_out = npy::read_npy<float>(raw_file);

    std::vector<float> data          = raw_out.data;
    std::vector<unsigned long> shape = raw_out.shape;

    EXPECT_EQ(shape.size(), 2);
    EXPECT_EQ(shape[1], 6);

    std::forward_list<ma_bbox_t> detections;
    for (size_t i = 0; i < shape[0]; i++) {
        ma_bbox_t box;

        box.x      = data[i * shape[1] + 0];
        box.y      = data[i * shape[1] + 1];
        box.w      = data[i * shape[1] + 2];
        box.h      = data[i * shape[1] + 3];
        box.score  = data[i * shape[1] + 4];
        box.target = data[i * shape[1] + 5];

        detections.emplace_front(box);
    }

    EXPECT_EQ(std::distance(detections.begin(), detections.end()), shape[0]);

    auto detections_multi_target = detections;
    ma::utils::nms(detections_multi_target, 0.2, 0.1, false, true);

    EXPECT_NE(std::distance(detections_multi_target.begin(), detections_multi_target.end()), 0);
    EXPECT_NE(std::distance(detections_multi_target.begin(), detections_multi_target.end()),
              shape[0]);

    std::string nms_file                 = npy_dir + "/nms_bboxes.npy";
    npy::npy_data nms_out                = npy::read_npy<float>(nms_file);
    std::vector<float> nms_data          = nms_out.data;
    std::vector<unsigned long> nms_shape = nms_out.shape;

    EXPECT_EQ(nms_shape.size(), 2);
    EXPECT_EQ(nms_shape[1], 6);
    EXPECT_EQ(nms_shape[0],
              std::distance(detections_multi_target.begin(), detections_multi_target.end()));

    i = 0;
    for (auto obj : detections_multi_target) {
        EXPECT_NEAR(obj.x, nms_data[i * nms_shape[1] + 0], 1e-6);
        EXPECT_NEAR(obj.y, nms_data[i * nms_shape[1] + 1], 1e-6);
        EXPECT_NEAR(obj.w, nms_data[i * nms_shape[1] + 2], 1e-6);
        EXPECT_NEAR(obj.h, nms_data[i * nms_shape[1] + 3], 1e-6);
        EXPECT_NEAR(obj.score, nms_data[i * nms_shape[1] + 4], 1e-6);
        auto target = static_cast<int>(nms_data[i * nms_shape[1] + 5]);
        EXPECT_EQ(obj.target, target);
        ++i;
    }

    auto detections_single_target = detections;
    ma::utils::nms(detections_single_target, 0.2, 0.1, false, false);

    EXPECT_NE(std::distance(detections_single_target.begin(), detections_single_target.end()), 0);
    EXPECT_NE(std::distance(detections_single_target.begin(), detections_single_target.end()),
              shape[0]);

    std::string nms_single_file                 = npy_dir + "/nms_bboxes_single_target.npy";
    npy::npy_data nms_single_out                = npy::read_npy<float>(nms_single_file);
    std::vector<float> nms_single_data          = nms_single_out.data;
    std::vector<unsigned long> nms_single_shape = nms_single_out.shape;

    EXPECT_EQ(nms_single_shape.size(), 2);
    EXPECT_EQ(nms_single_shape[1], 6);
    EXPECT_EQ(nms_single_shape[0],
              std::distance(detections_single_target.begin(), detections_single_target.end()));
    i = 0;
    for (auto obj : detections_single_target) {
        EXPECT_NEAR(obj.x, nms_single_data[i * nms_single_shape[1] + 0], 1e-6);
        EXPECT_NEAR(obj.y, nms_single_data[i * nms_single_shape[1] + 1], 1e-6);
        EXPECT_NEAR(obj.w, nms_single_data[i * nms_single_shape[1] + 2], 1e-6);
        EXPECT_NEAR(obj.h, nms_single_data[i * nms_single_shape[1] + 3], 1e-6);
        EXPECT_NEAR(obj.score, nms_single_data[i * nms_single_shape[1] + 4], 1e-6);
        auto target = static_cast<int>(nms_single_data[i * nms_single_shape[1] + 5]);
        EXPECT_EQ(obj.target, target);
        ++i;
    }
}

}  // namespace ma
