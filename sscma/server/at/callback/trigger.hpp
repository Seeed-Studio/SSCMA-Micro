#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "refactor_required.hpp"
#include "resource.hpp"

namespace ma::server::callback {

static bool parseAndSetTriggerRules(const std::string& trigger) {

    std::forward_list<std::shared_ptr<TriggerRule>> rules;

    for (size_t i = 0, j = 0; i < trigger.size(); i = j + 1) {
        j = trigger.find('|', i);
        if (j == std::string::npos) {
            j = trigger.size();
        }
        auto token = trigger.substr(i, j - i);
        auto ptr   = TriggerRule::create(token);
        if (ptr) {
            rules.push_front(std::move(ptr));
        } else {
            return false;
        }
    }

    trigger_rules_mutex.lock();
    trigger_rules = std::move(rules);
    trigger_rules_mutex.unlock();

    return true;
}

void configureTrigger(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder, bool called_by_event = false) {
    // [argv] 0: cmd, 1: trigger string
    // [trigger string] 0: class id, 1: condition, 2: threshold, 3: gpio pin, 4: default level, 5: trigger level | ...
    // [condition] 0: >, 1: <, 2: >=, 3: <=, 4: ==, 5: !=
    // [threshold] [0-100]: integer
    // [gpio pin] {1,2,3,21,41,42}: integer
    // [default level] {0,1}: integer
    // [trigger level] {0,1}: integer
    // example: "0,1,50,1,0,1|1,1,50,2,0,1"

    ma_err_t ret = MA_OK;
    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    if (!parseAndSetTriggerRules(argv[1])) {
        ret = MA_EINVAL;
        goto exit;
    }

    if (!called_by_event) {
        MA_STORAGE_SET_STR(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_TRIGGER_RULES, argv[1]);
    }

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("trigger_rules", argv[1]);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void getTrigger(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    std::string trigger;
    MA_STORAGE_GET_STR(static_resource->device->getStorage(), MA_STORAGE_KEY_TRIGGER_RULES, trigger, "");

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("trigger_rules", trigger);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void initDefaultTrigger(Encoder& encoder) {
    if (static_resource->device->getTransports().empty()) {
        MA_LOGD(MA_TAG, "No transport available");
        return;
    }
    auto& transport = static_resource->device->getTransports().front();
    if (!transport || !*transport) {
        MA_LOGD(MA_TAG, "Transport not available");
        return;
    }

    std::string trigger;
    MA_STORAGE_GET_STR(static_resource->device->getStorage(), MA_STORAGE_KEY_TRIGGER_RULES, trigger, "");

    std::vector<std::string> args{"INIT@TRIGGER", std::move(trigger)};
    configureTrigger(args, *transport, encoder, true);
}

}  // namespace ma::server::callback