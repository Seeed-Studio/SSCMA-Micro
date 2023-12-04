#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "core/el_types.h"
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
    WiFi()
        : _network(Device::get_device()->get_network()),
          _wifi_config(std::pair<wifi_sta_e, wifi_config_t>{wifi_sta_e::UNKNOWN, wifi_config_t{}}) {
        EL_ASSERT(_network);
    }

    ~WiFi() = default;

    void poll_from_supervisor() override {
        EL_LOGI("[SSCMA] WiFi::poll_from_supervisor()");

        if (_wifi_config.is_synchorized()) [[likely]] {
            const auto& config      = _wifi_config.load_last();
            auto        current_sta = sync_status_from_driver();
            if (current_sta < config.second.first) [[unlikely]]
                bring_up(config.second);
        } else {
            auto config = _wifi_config.load();
            if (set_down(config.second) && bring_up(config.second)) _wifi_config.synchorize(std::move(config));
        }
    }

    bool is_interface_up() const override { return is_wifi_joined(); }

    bool set_wifi_config(const wifi_config_t& wifi_config) {
        if (wifi_config.security_type > wifi_secu_type_e::NONE && std::strlen(wifi_config.passwd) < 8) [[unlikely]]
            return false;  // password too short
        _wifi_config.store(std::pair<wifi_sta_e, wifi_config_t>{
          std::strlen(wifi_config.name) ? wifi_sta_e::JOINED : wifi_sta_e::UNINTIALIZED, wifi_config});
        return true;
    }

    wifi_config_t get_wifi_config() const { return _wifi_config.load().second.second; }

    wifi_config_t get_last_wifi_config() const { return _wifi_config.load_last().second.second; }

    bool is_wifi_config_synchornized() const { return _wifi_config.is_synchorized(); }

    bool is_wifi_joined() const { return sync_status_from_driver() >= wifi_sta_e::JOINED; }

    // TODO: add driver implementation
    in4_info_t get_in4_info() const { return {}; }

    // TODO: add driver implementation
    in6_info_t get_in6_info() const { return {}; }

   protected:
    bool bring_up(const std::pair<wifi_sta_e, wifi_config_t>& config) {
        auto current_sta = sync_status_from_driver();

        EL_LOGI("[SSCMA] WiFi::bring_up() - current_sta: %d, target_sta: %d", current_sta, config.first);

        if (current_sta == wifi_sta_e::UNINTIALIZED) {
            if (current_sta >= config.first) return true;
            _network->init();  // driver init
            current_sta =
              try_ensure_wifi_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS, SSCMA_WIFI_POLL_RETRY);
        }

        if (current_sta == wifi_sta_e::IDLE) {
            if (current_sta >= config.first) return true;
            EL_LOGI("[SSCMA] WiFi::bring_up() - wifi name: %s", config.second.name);
            auto ret = _network->join(config.second.name, config.second.passwd);  // driver join
            if (ret != EL_OK) [[unlikely]]
                return false;
            current_sta =
              try_ensure_wifi_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS, SSCMA_WIFI_POLL_RETRY);
        }

        if (current_sta == wifi_sta_e::JOINED) {
            EL_LOGI("[SSCMA] WiFi::bring_up() - invoke_post_up_callbacks()");
            this->invoke_post_up_callbacks();
            return true;
        }

        return false;
    }

    bool set_down(const std::pair<wifi_sta_e, wifi_config_t>& config) {
        auto current_sta = sync_status_from_driver();

        EL_LOGI("[SSCMA] WiFi::set_down() - current_sta: %d, target_sta: %d", current_sta, config.first);

        if (current_sta == wifi_sta_e::BUSY) {
            if (current_sta < config.first) return true;
            EL_LOGI("[SSCMA] WiFi::set_down() - invoke_pre_down_callbacks()");
            this->invoke_pre_down_callbacks();
            current_sta = ensure_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS);
        }

        if (current_sta == wifi_sta_e::JOINED) {
            if (current_sta < config.first) return true;
            EL_LOGI("[SSCMA] WiFi::set_down() - driver quit()");
            auto ret = _network->quit();  // driver quit
            if (ret != EL_OK) [[unlikely]]
                return false;
            current_sta = ensure_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS);
        }

        if (current_sta == wifi_sta_e::IDLE) {
            if (current_sta < config.first) return true;
            EL_LOGI("[SSCMA] WiFi::set_down() - driver deinit()");
            _network->deinit();  // driver deinit
            current_sta = ensure_status_changed_from(current_sta, SSCMA_WIFI_POLL_DELAY_MS);
        }

        if (current_sta == wifi_sta_e::UNINTIALIZED) return true;

        return false;
    }

    wifi_sta_e try_ensure_wifi_status_changed_from(wifi_sta_e old_sta, uint32_t delay_ms, std::size_t n_times) {
        auto sta = sync_status_from_driver();
        for (std::size_t i = 0; (i < n_times) & (sta == old_sta); ++i) {
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

    SynchronizableObject<std::pair<wifi_sta_e, wifi_config_t>> _wifi_config;
};

}  // namespace interface

}  // namespace sscma
