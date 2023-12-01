#pragma once

#include <cstdint>
#include <cstring>
#include <list>
#include <string>
#include <utility>

#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "porting/el_device.h"
#include "porting/el_misc.h"
#include "porting/el_network.h"
#include "sscma/definations.hpp"
#include "sscma/prototypes.hpp"
#include "sscma/types.hpp"
#include "sscma/utility.hpp"

namespace sscma {

using namespace edgelab;

using namespace sscma::types;
using namespace sscma::prototypes;

namespace types {

typedef enum class wifi_sta_e { UNKNOWN = 0, UNINTIALIZED, IDLE, JOINED, BUSY } wifi_sta_e;

}  // namespace types

namespace interface {

class WiFi final : public Supervisable, public StatefulInterface {
   public:
    WiFi() : _network(Device::get_device()->get_network()), _wifi_config_lock(), _wifi_config_queue() {
        EL_ASSERT(_network);
        _wifi_config_queue.emplace_back(sync_status_from_driver(), wifi_config_t{});
    }

    ~WiFi() = default;

    void poll_from_supervisor() override {
        auto wifi_config        = std::pair<wifi_sta_e, wifi_config_t>{};
        bool wifi_config_synced = true;
        {
            const Guard guard(_wifi_config_lock);
            wifi_config = _wifi_config_queue.front();
            if (_wifi_config_queue.size() > 1) [[unlikely]] {
                wifi_config        = _wifi_config_queue.back();
                wifi_config_synced = false;
                while (_wifi_config_queue.size() > 1) _wifi_config_queue.pop_front();
            }
        }

        if (wifi_config_synced) [[likely]] {
            auto current_sta = sync_status_from_driver();
            if (current_sta < wifi_config.first) [[unlikely]]
                bring_up(wifi_config);
        } else {
            set_down(wifi_config);
            bring_up(wifi_config);
        }
    }

    bool is_interface_up() const override { return is_wifi_joined(); }

    bool set_wifi_config(const wifi_config_t& wifi_config) {
        if (wifi_config.security_type > wifi_secu_type_e::NONE && std::strlen(wifi_config.passwd) < 8) [[unlikely]]
            return false;  // password too short
        const Guard guard(_wifi_config_lock);
        _wifi_config_queue.emplace_back(std::strlen(wifi_config.name) ? wifi_sta_e::JOINED : wifi_sta_e::UNINTIALIZED,
                                        wifi_config);
        return true;
    }

    wifi_config_t get_wifi_config() const {
        const Guard guard(_wifi_config_lock);
        return _wifi_config_queue.front().second;
    }

    wifi_config_t get_last_wifi_config() const {
        const Guard guard(_wifi_config_lock);
        return _wifi_config_queue.back().second;
    }

    bool is_wifi_config_synchornized() const {
        const Guard guard(_wifi_config_lock);
        return _wifi_config_queue.size() <= 1;
    }

    bool is_wifi_joined() const { return sync_status_from_driver() >= wifi_sta_e::JOINED; }

    // TODO: add driver implementation
    in4_info_t get_in4_info() const { return {}; }

    // TODO: add driver implementation
    in6_info_t get_in6_info() const { return {}; }

   protected:
    void bring_up(const std::pair<wifi_sta_e, wifi_config_t>& config) {
        auto current_sta = sync_status_from_driver();

        if (current_sta == wifi_sta_e::UNINTIALIZED) {
            if (current_sta > config.first) return;
            _network->init();  // driver init
            current_sta =
              try_ensure_wifi_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS, SSCMA_WIFI_POLL_RETRY);
        }

        if (current_sta == wifi_sta_e::IDLE) {
            if (current_sta > config.first) return;
            auto ret = _network->join(config.second.name, config.second.passwd);  // driver join
            if (ret != EL_OK) [[unlikely]]
                return;
            current_sta =
              try_ensure_wifi_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS, SSCMA_WIFI_POLL_RETRY);
        }

        if (current_sta >= wifi_sta_e::JOINED) {
            this->invoke_post_up_callbacks();  // StatefulInterface::invoke_post_up_callbacks()
        }
    }

    void set_down(const std::pair<wifi_sta_e, wifi_config_t>& config) {
        auto current_sta = sync_status_from_driver();

        if (current_sta >= wifi_sta_e::BUSY) {
            if (current_sta < config.first) return;
            this->invoke_pre_down_callbacks();  // StatefulInterface::invoke_pre_down_callbacks()
            current_sta = ensure_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS);
        }

        if (current_sta == wifi_sta_e::JOINED) {
            if (current_sta < config.first) return;
            auto ret = _network->quit();  // driver quit
            if (ret != EL_OK) [[unlikely]]
                return;
            current_sta = ensure_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS);
        }

        if (current_sta == wifi_sta_e::IDLE) {
            if (current_sta < config.first) return;
            _network->deinit();  // driver deinit
            current_sta = ensure_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS);
        }
    }

    wifi_sta_e try_ensure_wifi_status_changed_from(wifi_sta_e old_sta, uint32_t delay_ms, std::size_t n_times) {
        auto sta = sync_status_from_driver();
        for (std::size_t i = 0; (i < n_times) & (sta != old_sta); ++i) {
            el_sleep(delay_ms);
            sta = sync_status_from_driver();
        }
        return sta;
    }

    wifi_sta_e ensure_status_changed_from(wifi_sta_e old_sta, uint32_t delay_ms) {
        auto sta = sync_status_from_driver();
        for (; sta == old_sta;) {
            el_sleep(delay_ms);
            sta = sync_status_from_driver();
        }
        return sta;
    }

    wifi_sta_e sync_status_from_driver() const {
        auto driver_sta = _network->status();
        switch (driver_sta) {
        case NETWORK_LOST:
            return wifi_sta_e::UNINTIALIZED;
        case NETWORK_IDLE:
            return wifi_sta_e::IDLE;
        case NETWORK_JOINED:
            return wifi_sta_e::JOINED;
        case NETWORK_CONNECTED:
            return wifi_sta_e::BUSY;
        default:
            return wifi_sta_e::UNKNOWN;
        }
    }

   private:
    Network* _network;

    Mutex                                           _wifi_config_lock;
    std::list<std::pair<wifi_sta_e, wifi_config_t>> _wifi_config_queue;
};

}  // namespace interface

}  // namespace sscma
