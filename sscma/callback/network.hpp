#pragma once

#include <cstring>
#include <functional>
#include <string>

#include "shared/network_utility.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

void set_wireless_network(const std::vector<std::string>&  argv,
                          bool                             called_by_event = false,
                          std::function<void(std::string)> on_joined_hook  = nullptr) {
    // disable network supervisor
    static_resource->enable_network_supervisor.store(false);

    // crate config from argv
    auto config      = wireless_network_config_t{};
    config.name_type = is_bssid(argv[1]) ? wireless_network_name_type_e::BSSID : wireless_network_name_type_e::SSID;
    std::strncpy(config.name, argv[1].c_str(), sizeof(config.name) - 1);
    config.security_type = static_cast<wireless_network_secu_type_e>(std::atol(argv[2].c_str()));
    std::strncpy(config.passwd, argv[3].c_str(), sizeof(config.passwd) - 1);

    int  conn_retry = SSCMA_WIRELESS_NETWORK_CONN_RETRY;
    auto ret        = EL_OK;
    auto sta        = static_resource->network->status();
    auto sta_old    = sta;

    // if called by event, just store the config to flash and give a direct reply
    if (!called_by_event) {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                      argv[0],
                                      "\", \"code\": ",
                                      std::to_string(ret),
                                      ", \"data\": ",
                                      wireless_network_config_2_json_str(config, false),
                                      "}\n")};
        static_resource->transport->send_bytes(ss.c_str(), ss.size());
    }

    // ensure the network is idle
    goto EnsureIdle;
EnsureIdleAgain:
    sta = try_ensure_network_status_changed(
      sta_old, SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS, SSCMA_WIRELESS_NETWORK_POLL_RETRY);
    if (sta == sta_old) [[unlikely]] {
        if (--conn_retry <= 0) [[unlikely]] {
            ret = EL_ETIMOUT;
            goto SyncAndReply;
        }
        goto EnsureIdleAgain;
    }
    sta_old = sta;  // update status
EnsureIdle:
    switch (sta) {
    case NETWORK_IDLE:
        break;

    case NETWORK_CONNECTED:
        // disconnect if connected (an unsubscribe all API is needed)
        ret = static_resource->network->disconnect();
        if (ret != EL_OK) [[unlikely]]
            goto SyncAndReply;
        goto EnsureIdleAgain;

    case NETWORK_JOINED:
        // quit if joined
        ret = static_resource->network->quit();
        if (ret != EL_OK) [[unlikely]]
            goto SyncAndReply;
        goto EnsureIdleAgain;

    case NETWORK_LOST:
        // init again if lost
        static_resource->network->init();  // Question: is init synchronous?
        goto EnsureIdleAgain;

    default:
        ret = EL_ELOG;
        goto SyncAndReply;
    }

    // just deinit and return if the network name is empty
    if (argv[1].empty()) [[unlikely]] {
        static_resource->network->deinit();  // Question: is deinit synchronous?
        sta = try_ensure_network_status_changed(
          sta_old, SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS, SSCMA_WIRELESS_NETWORK_POLL_RETRY);
        if (sta == sta_old) [[unlikely]]
            ret = EL_ETIMOUT;
        else if (sta != NETWORK_LOST) [[unlikely]]
            ret = EL_EIO;
        goto SyncAndReply;
    }

    // try serveral times to join the network
    conn_retry = SSCMA_WIRELESS_NETWORK_CONN_RETRY;  // reset retry counter
    goto TryJoin;
TryJoinAgain:
    sta = try_ensure_network_status_changed(
      sta_old, SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS, SSCMA_WIRELESS_NETWORK_POLL_RETRY);
    if (sta == sta_old) [[unlikely]] {
        if (--conn_retry <= 0) [[unlikely]] {
            ret = EL_ETIMOUT;
            goto SyncAndReply;
        }
        goto TryJoinAgain;
    }
    sta_old = sta;  // update status
TryJoin:
    switch (sta) {
    case NETWORK_IDLE:
        // try to join to the network
        ret = static_resource->network->join(config.name, config.passwd);
        if (ret != EL_OK) [[unlikely]]
            goto SyncAndReply;
        goto TryJoinAgain;

    case NETWORK_JOINED:
        break;

    case NETWORK_LOST:
    case NETWORK_CONNECTED:
        ret = EL_EIO;
        goto SyncAndReply;

    default:
        ret = EL_ELOG;
        goto SyncAndReply;
    }

    // sync status before hook functions
    if (!called_by_event) static_resource->target_network_status = sta;

    // call hook function
    if (on_joined_hook) on_joined_hook(argv[0]);

    // never sync status after hook functions
    goto Reply;

SyncAndReply:
    // sync status
    if (!called_by_event) static_resource->target_network_status = sta;

Reply:
    // enable network supervisor
    static_resource->enable_network_supervisor.store(true);

    bool        joined = sta == NETWORK_JOINED || sta == NETWORK_CONNECTED;
    const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"joined\": ",
                                  std::to_string(joined ? 1 : 0),
                                  ", \"config\": ",
                                  wireless_network_config_2_json_str(config),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_wireless_network(const std::string& cmd) {
    auto sta    = static_resource->network->status();
    bool joined = sta == NETWORK_JOINED || sta == NETWORK_CONNECTED;
    auto config = wireless_network_config_t{};
    auto ret    = static_resource->storage->get(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"joined\": ",
                                  std::to_string(joined ? 1 : 0),
                                  ", \"config\": ",
                                  wireless_network_config_2_json_str(config),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
