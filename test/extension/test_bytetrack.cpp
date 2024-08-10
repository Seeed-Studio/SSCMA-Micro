#include <gtest/gtest.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "3rdparty/npy.hpp"

#include "core/ma_types.h"
#include "extension/bytetrack/byte_tracker.h"

namespace ma {

TEST(EXTENSION, ByteTrackerFunctional) {

    std::string current_path = __FILE__;
    current_path             = current_path.substr(0, current_path.find_last_of("/"));

    std::cout << "current_path: " << current_path << std::endl;
    std::string npy_dir = current_path + "/test_data";

    BYTETracker tracker(30, 30, 0.25, 0.35, 0.8);

    for (size_t i = 0; i < 10; ++i) {
        std::string raw_file  = npy_dir + "/raw_out_" + std::to_string(i) + ".npy";
        npy::npy_data raw_out = npy::read_npy<float>(raw_file);

        std::vector<float> data          = raw_out.data;
        std::vector<unsigned long> shape = raw_out.shape;

        EXPECT_EQ(shape.size(), 2);
        EXPECT_EQ(shape[1], 6);

        std::vector<ma_bbox_t> detections;

        for (size_t j = 0; j < shape[0]; ++j) {
            ma_bbox_t box;

            box.x      = data[j * shape[1] + 0];
            box.y      = data[j * shape[1] + 1];
            box.w      = data[j * shape[1] + 2];
            box.h      = data[j * shape[1] + 3];
            box.score  = data[j * shape[1] + 4];
            box.target = data[j * shape[1] + 5];

            detections.push_back(box);
        }

        EXPECT_EQ(detections.size(), shape[0]);

        auto tracks = tracker.inplace_update(detections);
        {
            std::unordered_set<int> tracked_ids(tracks.begin(), tracks.end());
            EXPECT_EQ(tracked_ids.size(), tracks.size());
        }
        EXPECT_EQ(tracks.size(), detections.size());

        std::string tracked_file  = npy_dir + "/tracked_out_" + std::to_string(i) + ".npy";
        npy::npy_data tracked_out = npy::read_npy<float>(tracked_file);

        std::vector<float> tracked_data          = tracked_out.data;
        std::vector<unsigned long> tracked_shape = tracked_out.shape;

        EXPECT_EQ(tracked_shape.size(), 2);
        EXPECT_EQ(tracked_shape[1], 7);

        EXPECT_EQ(tracked_shape[0], tracks.size());

        for (size_t j = 0; j < tracked_shape[0]; ++j) {
            auto x      = tracked_data[j * tracked_shape[1] + 0];
            auto y      = tracked_data[j * tracked_shape[1] + 1];
            auto w      = tracked_data[j * tracked_shape[1] + 2];
            auto h      = tracked_data[j * tracked_shape[1] + 3];
            auto score  = tracked_data[j * tracked_shape[1] + 4];
            auto target = static_cast<int>(tracked_data[j * tracked_shape[1] + 5]);
            auto id     = static_cast<int>(tracked_data[j * tracked_shape[1] + 6]);

            EXPECT_NEAR(detections[j].x, x, 1e-2);
            EXPECT_NEAR(detections[j].y, y, 1e-2);
            EXPECT_NEAR(detections[j].w, w, 1e-2);
            EXPECT_NEAR(detections[j].h, h, 1e-2);
            EXPECT_NEAR(detections[j].score, score, 1e-2);
            EXPECT_EQ(detections[j].target, target);
            EXPECT_EQ(tracks[j], id);
        }
    }
}

}  // namespace ma
