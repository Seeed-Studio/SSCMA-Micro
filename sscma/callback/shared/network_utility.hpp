#pragma once

#include <cstdint>

#include "core/el_types.h"
#include "porting/el_misc.h"
#include "porting/el_network.h"
#include "sscma/static_resource.hpp"

namespace sscma::utility {

el_net_sta_t try_ensure_network_status_changed(el_net_sta_t old_sta, uint32_t delay_ms, std::size_t n_times) {
    for (std::size_t i = 0; i < n_times; ++i) {
        el_sleep(delay_ms);
        auto sta = static_resource->network->status();
        if (sta != old_sta) [[likely]]
            return sta;
    }
    return old_sta;
}

}  // namespace sscma::utility
