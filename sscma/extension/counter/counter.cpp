
#include <cstdio>

#include "counter.h"


Counter::Counter(int32_t frame_rate)
    : frame_rate(frame_rate), countAB(0), countBA(0), splitter({0, 0, 0, 0}) {}
Counter::~Counter() {}

int Counter::getSide(int16_t x, int16_t y) {
    float value = (x - splitter[0]) * (splitter[3] - splitter[1]) -
        (y - splitter[1]) * (splitter[2] - splitter[0]);
    if (value > 0) {
        return 1;
    } else if (value < 0) {
        return -1;
    }
    return 0;
}

void Counter::update(int32_t id, int16_t x, int16_t y) {
    if (id != -1) {
        if (objects.find(id) == objects.end()) {
            objects[id]      = object{.id = id, .x = x, .y = y, .count = 0};
            objects[id].side = getSide(x, y);
        } else {
            object& obj  = objects[id];
            obj.x        = x;
            obj.y        = y;
            obj.count    = 0;
            int32_t side = getSide(x, y);
            if (obj.side == -1 && side == 1) {
                countAB += 1;
            } else if (obj.side == 1 && side == -1) {
                countBA += 1;
            }
            obj.side = side;
        }
    }
    for (auto& obj : objects) {
        obj.second.count += 1;
        if (obj.second.count > frame_rate) {
            obj.second.count = 0;
            objects.erase(obj.first);  // object lost
        }
    }
}

void Counter::clear() {
    objects.clear();
    countAB = 0;
    countBA = 0;
}

void Counter::setSplitter(std::vector<int16_t> splitter) {
    if(splitter.size() < 4){
        return;
    }
    this->splitter.clear();
    this->splitter = splitter;
}

std::vector<int32_t> Counter::get() {
    std::vector<int32_t> count;
    int32_t countA = 0;
    int32_t countB = 0;

    for (auto& obj : objects) {
        if (obj.second.side == 1) {
            countA += 1;
        } else if (obj.second.side == -1) {
            countB += 1;
        }
    }
    count.push_back(countA);
    count.push_back(countB);
    count.push_back(countAB);
    count.push_back(countBA);
    return count;
}

std::vector<int16_t> Counter::getSplitter() {
    return splitter;
}
