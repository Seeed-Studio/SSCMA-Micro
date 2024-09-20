#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma::server::callback {

void getAvailableAlgorithms(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    // Need to refactor the algorithms
    // - A registry of algorithms is needed and the registry should be able to provide the list of available algorithms
    // - Each algorithm in the registry should be able to provide the information about itself
}

void configureAlgorithm(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {}

void getAlgorithmInfo(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {}

}  // namespace ma::server::callback
