/*
 * MIT License
 * Copyright (c) 2024 lynnl4 Seeed Technology Co.,Ltd
 *
 */

#ifndef _COUNTER_H_
#define _COUNTER_H_

#include <stdint.h>
#include <unordered_map>
#include <vector>

class Counter {
public:
    Counter(int32_t frame_rate = 10);
    ~Counter();
    void update(int32_t id, int16_t x, int16_t y);
    void clear();
    struct object {
        int32_t id;
        int16_t x;
        int16_t y;
        int16_t side;
        int32_t count;
    };
    void setSplitter(std::vector<int16_t> splitter);
    std::vector<int16_t> getSplitter();
    std::vector<int32_t> get();

protected:
    int getSide(int16_t x, int16_t y);

private:
    int32_t frame_rate;
    int32_t countAB;  // A -> B
    int32_t countBA;  // B -> A
    std::vector<int16_t> splitter;
    std::unordered_map<int32_t, object> objects;
};

#endif
