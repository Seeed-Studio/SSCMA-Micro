#include "ma_codec_json.h"

#include <cJSON.h>

namespace ma {

static const char* TAG = "ma::codec::JSON";

EncoderJSON::EncoderJSON() : m_mutex(false) {

    m_root = nullptr;
    m_data = nullptr;

    static bool s_inited       = false;
    static cJSON_Hooks g_hooks = {
        .malloc_fn = ma_malloc,
        .free_fn   = ma_free,
    };
    if (!s_inited) {  // only init once
        cJSON_InitHooks(&g_hooks);
        s_inited = true;
    }
}

EncoderJSON::~EncoderJSON() {
    if (m_root) {
        cJSON_Delete(m_root);
    }
}

EncoderJSON::operator bool() const {
    return m_root != nullptr;
}

ma_err_t EncoderJSON::begin() {
    m_mutex.lock();
    ma_err_t ret = reset();
    if (ret != MA_OK) [[unlikely]] {
        goto err;
    }
    m_root = cJSON_CreateObject();
    if (m_root == nullptr) [[unlikely]] {
        ret = MA_ENOMEM;
        goto err;
    }
    m_data = m_root;

    return MA_OK;
err:
    m_mutex.unlock();
    return ret;
}

ma_err_t EncoderJSON::begin(ma_msg_type_t type, ma_err_t code, const std::string& name) {
    m_mutex.lock();
    ma_err_t ret = reset();

    if (ret != MA_OK) [[unlikely]] {
        goto err;
    }

    m_root = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(m_root, "type", type) == nullptr || cJSON_AddStringToObject(m_root, "name", name.c_str()) == nullptr || cJSON_AddNumberToObject(m_root, "code", code) == nullptr)
        [[unlikely]] {
        reset();
        ret = MA_FAILED;
        goto err;
    }

    m_data = cJSON_AddObjectToObject(m_root, "data");
    if (m_data == nullptr) [[unlikely]] {
        reset();
        ret = MA_FAILED;
        goto err;
    }

    return MA_OK;

err:
    m_mutex.unlock();
    return ret;
}

ma_err_t EncoderJSON::begin(ma_msg_type_t type, ma_err_t code, const std::string& name, const std::string& data) {
    m_mutex.lock();
    ma_err_t ret = reset();
    if (ret != MA_OK) [[unlikely]] {
        m_mutex.unlock();
        return ret;
    }
    m_root = cJSON_CreateObject();
    if (cJSON_AddNumberToObject(m_root, "type", type) == nullptr || cJSON_AddStringToObject(m_root, "name", name.c_str()) == nullptr || cJSON_AddNumberToObject(m_root, "code", code) == nullptr ||
        cJSON_AddStringToObject(m_root, "data", data.c_str()) == nullptr) [[unlikely]] {
        reset();
        m_mutex.unlock();
        return MA_FAILED;
    }
    return MA_OK;
}

ma_err_t EncoderJSON::begin(ma_msg_type_t type, ma_err_t code, const std::string& name, uint64_t data) {
    m_mutex.lock();
    ma_err_t ret = reset();
    if (ret != MA_OK) [[unlikely]] {
        m_mutex.unlock();
        return ret;
    }
    m_root = cJSON_CreateObject();
    if (cJSON_AddNumberToObject(m_root, "type", type) == nullptr || cJSON_AddStringToObject(m_root, "name", name.c_str()) == nullptr || cJSON_AddNumberToObject(m_root, "code", code) == nullptr ||
        cJSON_AddNumberToObject(m_root, "data", data) == nullptr) [[unlikely]] {
        reset();
        m_mutex.unlock();
        return MA_FAILED;
    }
    return MA_OK;
}

ma_err_t EncoderJSON::end() {
    ma_err_t ret = MA_OK;
    char* str    = nullptr;
    if (m_root == nullptr) [[unlikely]] {
        ret = MA_EPERM;
        goto exit;
    }
    str = cJSON_PrintUnformatted(m_root);
    if (!str) [[unlikely]] {
        ret = MA_ENOMEM;
        goto exit;
    }
    m_string.clear();
    m_string.shrink_to_fit();
    m_string += "\r";  // prefix
    m_string += str;
    m_string += "\n";  // suffix
    ma_free(str);

exit:
    m_mutex.unlock();
    return ret;
}

const std::string& EncoderJSON::toString() const {
    return m_string;
}

const void* EncoderJSON::data() const {
    return m_string.c_str();
}

const size_t EncoderJSON::size() const {
    return m_string.size();
}

ma_err_t EncoderJSON::reset() {
    m_string.clear();
    m_string.shrink_to_fit();
    if (m_root) {
        cJSON_Delete(m_root);
        m_root = nullptr;
    }
    return MA_OK;
}

ma_err_t EncoderJSON::remove(const std::string& key) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) == nullptr) {
        return MA_ENOENT;
    }
    cJSON_DeleteItemFromObject(m_data, key.c_str());
    return MA_OK;
}

ma_err_t EncoderJSON::write(const std::string& key, int8_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, int16_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, int32_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, int64_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, uint8_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, uint16_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, uint32_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, uint64_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, float value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, double value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, const std::string& value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddStringToObject(m_data, key.c_str(), value.c_str()) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t EncoderJSON::write(const std::string& key, ma_model_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }

    cJSON* item = cJSON_AddObjectToObject(m_data, key.c_str());
    if (item == nullptr) {
        return MA_FAILED;
    }

    cJSON_AddItemToObject(item, "id", cJSON_CreateNumber(value.id));
    cJSON_AddItemToObject(item, "type", cJSON_CreateNumber(value.type));
    cJSON_AddItemToObject(item, "size", cJSON_CreateNumber(value.size));
#if MA_USE_FILESYSTEM
    cJSON_AddItemToObject(item, "name", cJSON_CreateString(static_cast<const char*>(value.name)));
    cJSON_AddItemToObject(item, "address", cJSON_CreateString(static_cast<const char*>(value.addr)));
#else

#if MA_ARCH_64
    cJSON_AddItemToObject(item, "name", cJSON_CreateNumber(reinterpret_cast<uint64_t>(value.name)));
    cJSON_AddItemToObject(item, "address", cJSON_CreateNumber(reinterpret_cast<uint64_t>(value.addr)));
#else
    cJSON_AddItemToObject(item, "name", cJSON_CreateNumber(reinterpret_cast<uint32_t>(value.name)));
    cJSON_AddItemToObject(item, "address", cJSON_CreateNumber(reinterpret_cast<uint32_t>(value.addr)));
#endif
#endif

    return MA_OK;
}

ma_err_t EncoderJSON::write(ma_perf_t value) {
    if (cJSON_GetObjectItem(m_data, "perf") != nullptr) {
        return MA_EEXIST;
    }
    cJSON* item = cJSON_AddArrayToObject(m_data, "perf");
    if (item == nullptr) {
        return MA_FAILED;
    }
    cJSON_AddItemToArray(item, cJSON_CreateNumber(value.preprocess));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(value.inference));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(value.postprocess));
    return MA_OK;
}

ma_err_t EncoderJSON::write(const std::forward_list<ma_class_t>& value) {
    if (cJSON_GetObjectItem(m_data, "classes") != nullptr) {
        return MA_EEXIST;
    }
    cJSON* array = cJSON_AddArrayToObject(m_data, "classes");
    if (array == nullptr) {
        MA_LOGD(TAG, "cJSON_AddArrayToObject failed");
        return MA_FAILED;
    }
    for (auto it = value.begin(); it != value.end(); it++) {
        cJSON* item = cJSON_CreateArray();
        if (item == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->score));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->target));
        cJSON_AddItemToArray(array, item);
    }
    return MA_OK;
}

ma_err_t EncoderJSON::write(const std::forward_list<ma_keypoint3f_t>& value) {
    cJSON* array = cJSON_CreateArray();
    cJSON_ReplaceItemInObjectCaseSensitive(m_root, "keypoints", array);
    if (array == nullptr) {
        return MA_FAILED;
    }
    for (auto it = value.begin(); it != value.end(); it++) {
        cJSON* item = cJSON_CreateArray();
        if (item == nullptr) {
            return MA_FAILED;
        }
        // box
        cJSON* box = cJSON_CreateArray();
        if (box == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToArray(box, cJSON_CreateNumber(it->box.x));
        cJSON_AddItemToArray(box, cJSON_CreateNumber(it->box.y));
        cJSON_AddItemToArray(box, cJSON_CreateNumber(it->box.w));
        cJSON_AddItemToArray(box, cJSON_CreateNumber(it->box.h));
        cJSON_AddItemToArray(box, cJSON_CreateNumber(it->box.score));
        cJSON_AddItemToArray(box, cJSON_CreateNumber(it->box.target));
        cJSON_AddItemToArray(item, box);
        cJSON_AddItemToArray(array, item);
        // pts
        cJSON* pts = cJSON_CreateArray();
        if (pts == nullptr) {
            return MA_FAILED;
        }
        for (const auto& pt : it->pts) {
            cJSON_AddItemToArray(pts, cJSON_CreateNumber(pt.x));
            cJSON_AddItemToArray(pts, cJSON_CreateNumber(pt.y));
            cJSON_AddItemToArray(pts, cJSON_CreateNumber(pt.z));
        }
        cJSON_AddItemToArray(item, pts);
    }

    return MA_OK;
}


ma_err_t EncoderJSON::write(const in4_info_t& value) {
    if (cJSON_GetObjectItem(m_data, "in4_info") != nullptr) {
        return MA_EEXIST;
    }
    cJSON* item = cJSON_CreateObject();
    if (item == nullptr) {
        return MA_FAILED;
    }
    std::string str = value.ip.to_str();
    cJSON_AddItemToObject(item, "ip", cJSON_CreateString(str.c_str()));
    str = value.netmask.to_str();
    cJSON_AddItemToObject(item, "netmask", cJSON_CreateString(str.c_str()));
    str = value.gateway.to_str();
    cJSON_AddItemToObject(item, "gateway", cJSON_CreateString(str.c_str()));
    cJSON_AddItemToObject(m_data, "in4_info", item);


    return MA_OK;
}

ma_err_t EncoderJSON::write(const in6_info_t& value) {
    if (cJSON_GetObjectItem(m_data, "in6_info") != nullptr) {
        return MA_EEXIST;
    }
    cJSON* item = cJSON_CreateObject();
    if (item == nullptr) {
        return MA_FAILED;
    }
    std::string str = value.ip.to_str();
    cJSON_AddItemToObject(item, "ip", cJSON_CreateString(str.c_str()));
    cJSON_AddItemToObject(m_data, "in6_info", item);
    return MA_OK;
}

ma_err_t EncoderJSON::write(const ma_wifi_config_t& value, int* stat) {

    cJSON* item = m_data;
    if (stat) {
        item = cJSON_CreateObject();
    }
    if (item == nullptr) {
        return MA_FAILED;
    }

    std::string str = value.ssid;
    if (str.size() < 1) {
        str = value.bssid;
    }

    cJSON_AddItemToObject(item, "name", cJSON_CreateString(str.c_str()));
    cJSON_AddItemToObject(item, "security", cJSON_CreateNumber(value.security));
    cJSON_AddItemToObject(item, "password", cJSON_CreateString(value.password));

    if (stat) {
        cJSON_AddItemToObject(m_data, "config", item);
        cJSON_AddItemToObject(m_data, "status", cJSON_CreateNumber(*stat));
    }

    return MA_OK;
}

ma_err_t EncoderJSON::write(int algo_id, int cat, int input_from, int tscore, int tiou) {
    cJSON* item = cJSON_CreateObject();
    if (item == nullptr) {
        return MA_FAILED;
    }

    cJSON_AddItemToObject(item, "type", cJSON_CreateNumber(algo_id));
    cJSON_AddItemToObject(item, "category", cJSON_CreateNumber(cat));
    cJSON_AddItemToObject(item, "input_from", cJSON_CreateNumber(input_from));

    cJSON* cfg = cJSON_CreateObject();
    if (cfg == nullptr) {
        return MA_FAILED;
    }

    cJSON_AddItemToObject(cfg, "tscore", cJSON_CreateNumber(tscore));
    cJSON_AddItemToObject(cfg, "tiou", cJSON_CreateNumber(tiou));

    cJSON_AddItemToObject(item, "config", cfg);

    cJSON_AddItemToObject(m_data, "algorithm", item);

    return MA_OK;
}


ma_err_t EncoderJSON::write(const std::forward_list<ma_point_t>& value) {

    if (cJSON_GetObjectItem(m_data, "points") != nullptr) {
        return MA_EEXIST;
    }
    cJSON* array = cJSON_AddArrayToObject(m_data, "points");
    if (array == nullptr) {
        return MA_FAILED;
    }
    for (auto it = value.begin(); it != value.end(); it++) {
        cJSON* item = cJSON_CreateArray();
        if (item == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->x));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->y));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->score));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->target));
        cJSON_AddItemToArray(array, item);
    }
    return MA_OK;
}
ma_err_t EncoderJSON::write(const std::forward_list<ma_bbox_t>& value) {
    if (cJSON_GetObjectItem(m_data, "boxes") != nullptr) {
        return MA_EEXIST;
    }
    if (cJSON_GetObjectItem(m_data, "boxes")) {
        return MA_EBUSY;
    }
    cJSON* array = cJSON_AddArrayToObject(m_data, "boxes");
    if (array == nullptr) {
        return MA_FAILED;
    }
    for (auto it = value.begin(); it != value.end(); it++) {
        cJSON* item = cJSON_CreateArray();
        if (item == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->x));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->y));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->w));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->h));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->score));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->target));
        cJSON_AddItemToArray(array, item);
    }
    return MA_OK;
}

ma_err_t EncoderJSON::write(const std::vector<ma_model_t>& value) {
    cJSON* array = cJSON_AddArrayToObject(m_data, "models");
    if (array == nullptr) {
        return MA_FAILED;
    }
    for (auto it = value.begin(); it != value.end(); it++) {
        cJSON* item = cJSON_CreateObject();
        if (item == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToObject(item, "id", cJSON_CreateNumber(it->id));
        cJSON_AddItemToObject(item, "type", cJSON_CreateNumber(it->type));
        cJSON_AddItemToObject(item, "size", cJSON_CreateNumber(it->size));
#if MA_USE_FILESYSTEM
        cJSON_AddItemToObject(item, "name", cJSON_CreateString(static_cast<const char*>(it->name)));
        cJSON_AddItemToObject(item, "address", cJSON_CreateString(static_cast<const char*>(it->addr)));
#else
#ifdef MA_ARCH_64
        cJSON_AddItemToObject(item, "name", cJSON_CreateNumber(reinterpret_cast<uint64_t>(it->name)));
        cJSON_AddItemToObject(item, "address", cJSON_CreateNumber(reinterpret_cast<uint64_t>(it->addr)));
#else
        cJSON_AddItemToObject(item, "name", cJSON_CreateNumber(reinterpret_cast<uint32_t>(it->name)));
        cJSON_AddItemToObject(item, "address", cJSON_CreateNumber(reinterpret_cast<uint32_t>(it->addr)));
#endif
#endif

        cJSON_AddItemToArray(array, item);
    }
    return MA_OK;
}

ma_err_t EncoderJSON::write(const std::vector<ma::Sensor*>& value) {
    cJSON* array = cJSON_AddArrayToObject(m_data, "sensors");
    if (array == nullptr) {
        return MA_FAILED;
    }
    for (auto it = value.begin(); it != value.end(); it++) {
        cJSON* item = cJSON_CreateObject();
        if (item == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToObject(item, "id", cJSON_CreateNumber((*it)->getID()));
        const auto& type = ma::Sensor::__repr__((*it)->getType());
        cJSON_AddItemToObject(item, "type", cJSON_CreateString(type.c_str()));
        cJSON_AddItemToObject(item, "initialized", cJSON_CreateBool(static_cast<bool>(*(*it))));
        cJSON* preset_items = cJSON_CreateArray();
        const auto& presets = (*it)->availablePresets();
        size_t j            = 0;
        for (auto pit = presets.begin(); pit != presets.end(); ++pit) {
            cJSON* preset_item = cJSON_CreateObject();
            cJSON_AddItemToObject(preset_item, "id", cJSON_CreateNumber(j++));
            cJSON_AddItemToObject(preset_item, "description", cJSON_CreateString(pit->description));
            cJSON_AddItemToArray(preset_items, preset_item);
        }
        cJSON_AddItemToObject(item, "presets", preset_items);
        const auto& current_preset = (*it)->currentPreset();
        cJSON_AddItemToObject(item, "current_preset_index", cJSON_CreateNumber((*it)->currentPresetIdx()));
        cJSON_AddItemToArray(array, item);
    }
    return MA_OK;
}

ma_err_t EncoderJSON::write(const std::string& key, const char* buffer, size_t size) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }

    cJSON* item = cJSON_CreateStringReference(buffer);
    if (item == nullptr) {
        return MA_FAILED;
    }
    cJSON_AddItemToObject(m_data, key.c_str(), item);
    return MA_OK;
}

ma_err_t EncoderJSON::write(const Sensor* value, size_t preset) {
    cJSON* array = cJSON_AddArrayToObject(m_data, "sensors");
    if (array == nullptr || value == nullptr) {
        return MA_FAILED;
    }

    const auto* it = value;

    cJSON* item = cJSON_CreateObject();
    if (item == nullptr) {
        return MA_FAILED;
    }
    cJSON_AddItemToObject(item, "id", cJSON_CreateNumber(it->getID()));
    const auto& type = ma::Sensor::__repr__(it->getType());
    cJSON_AddItemToObject(item, "type", cJSON_CreateString(type.c_str()));
    cJSON_AddItemToObject(item, "initialized", cJSON_CreateBool(static_cast<bool>(*it)));
    const auto& presets = it->availablePresets();
    size_t j            = 0;
    for (auto pit = presets.begin(); pit != presets.end(); ++pit, ++j) {
        if (j != preset) {
            continue;
        }
        cJSON* preset_item = cJSON_CreateObject();
        cJSON_AddItemToObject(preset_item, "id", cJSON_CreateNumber(j));
        cJSON_AddItemToObject(preset_item, "description", cJSON_CreateString(pit->description));
        cJSON_AddItemToObject(item, "current_preset", preset_item);
    }

    const auto& current_preset = it->currentPreset();
    cJSON_AddItemToArray(array, item);
    return MA_OK;
}


ma_err_t EncoderJSON::write(const ma_mqtt_config_t& value, int* stat) {

    cJSON* item = m_data;
    if (stat) {
        item = cJSON_CreateObject();
    }
    if (item == nullptr) {
        return MA_FAILED;
    }

    cJSON_AddItemToObject(item, "address", cJSON_CreateString(value.host));
    cJSON_AddItemToObject(item, "port", cJSON_CreateNumber(value.port));
    cJSON_AddItemToObject(item, "username", cJSON_CreateString(value.username));
    cJSON_AddItemToObject(item, "password", cJSON_CreateString(value.password));
    cJSON_AddItemToObject(item, "client_id", cJSON_CreateString(value.client_id));
    cJSON_AddItemToObject(item, "use_ssl", cJSON_CreateNumber((int)value.use_ssl));

    if (stat) {
        cJSON_AddItemToObject(m_data, "config", item);
        cJSON_AddItemToObject(m_data, "status", cJSON_CreateNumber(*stat));
    }

    return MA_OK;
}

ma_err_t EncoderJSON::write(const ma_mqtt_topic_config_t& value) {
    cJSON* item = cJSON_CreateObject();
    if (item == nullptr) {
        return MA_FAILED;
    }

    cJSON_AddItemToObject(item, "pub_topic", cJSON_CreateString(value.pub_topic));
    cJSON_AddItemToObject(item, "pub_qos", cJSON_CreateNumber((int)value.pub_qos));
    cJSON_AddItemToObject(item, "sub_topic", cJSON_CreateString(value.sub_topic));
    cJSON_AddItemToObject(item, "sub_qos", cJSON_CreateNumber((int)value.sub_qos));

    cJSON_AddItemToObject(m_data, "config", item);

    return MA_OK;
}


DecoderJSON::DecoderJSON() : m_root(nullptr), m_mutex(false) {}

DecoderJSON::~DecoderJSON() {
    if (m_root != nullptr) {
        cJSON_Delete(m_root);
        m_root = nullptr;
    }
}

DecoderJSON::operator bool() const {
    return m_root != nullptr;
}

ma_err_t DecoderJSON::begin(const std::string& data) {
    return begin(data.c_str(), data.size());
}

ma_err_t DecoderJSON::begin(const void* data, size_t size) {
    ma_err_t ret = MA_OK;
    cJSON* item  = nullptr;
    m_mutex.lock();
    m_root = cJSON_Parse(static_cast<const char*>(data));
    if (m_root == nullptr) {
        m_mutex.unlock();
        return MA_FAILED;
    }
    m_data = cJSON_GetObjectItem(m_root, "data");
    if (m_data == nullptr) {
        ret = MA_FAILED;
        goto err;
    }

    item = cJSON_GetObjectItem(m_root, "type");

    if (item == nullptr || item->type != cJSON_Number) {
        ret = MA_FAILED;
        goto err;
    }

    m_type = static_cast<ma_msg_type_t>(item->valueint);

    item = cJSON_GetObjectItem(m_root, "code");
    if (item == nullptr || item->type != cJSON_Number) {
        ret = MA_FAILED;
        goto err;
    }

    m_code = static_cast<ma_err_t>(item->valueint);

    item = cJSON_GetObjectItem(m_root, "name");
    if (item == nullptr || item->type != cJSON_String) {
        ret = MA_FAILED;
        goto err;
    }

    m_name = std::string(item->valuestring);

    return MA_OK;

err:
    if (m_root != nullptr) {
        cJSON_Delete(m_root);
    }
    m_mutex.unlock();
    return ret;
}

ma_err_t DecoderJSON::end() {
    if (m_root != nullptr) {
        cJSON_Delete(m_root);
        m_root = nullptr;
    }
    m_mutex.unlock();
    return MA_OK;
}

ma_msg_type_t DecoderJSON::type() const {
    return m_type;
}

ma_err_t DecoderJSON::code() const {
    return m_code;
}

std::string DecoderJSON::name() const {
    return m_name;
}

ma_err_t DecoderJSON::read(const std::string& key, int8_t& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = item->valueint;
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, int16_t& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = item->valueint;
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, int32_t& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = item->valueint;
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, int64_t& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = item->valuedouble;
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, uint8_t& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = static_cast<uint8_t>(item->valueint);
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, uint16_t& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = static_cast<uint16_t>(item->valueint);
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, uint32_t& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = static_cast<uint32_t>(item->valueint);
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, uint64_t& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = static_cast<uint64_t>(item->valuedouble);
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, float& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = static_cast<float>(item->valuedouble);
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, double& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_Number) {
        return MA_FAILED;
    }
    value = item->valuedouble;
    return MA_OK;
}

ma_err_t DecoderJSON::read(const std::string& key, std::string& value) const {
    cJSON* item = cJSON_GetObjectItem(m_data, key.c_str());
    if (item == nullptr || item->type != cJSON_String) {
        return MA_FAILED;
    }
    value = std::string(item->valuestring);
    return MA_OK;
}

ma_err_t DecoderJSON::read(ma_perf_t& value) {
    cJSON* item = cJSON_GetObjectItem(m_data, "perf");
    if (item == nullptr || item->type != cJSON_Array || cJSON_GetArraySize(item) != 3) {
        return MA_FAILED;
    }

    value.preprocess  = cJSON_GetArrayItem(item, 0)->valueint;
    value.inference   = cJSON_GetArrayItem(item, 1)->valueint;
    value.postprocess = cJSON_GetArrayItem(item, 2)->valueint;
    return MA_OK;
};
ma_err_t DecoderJSON::read(std::forward_list<ma_class_t>& value) {
    value.clear();
    cJSON* item = cJSON_GetObjectItem(m_data, "classes");
    if (item == nullptr || item->type != cJSON_Array) {
        return MA_FAILED;
    }

    for (cJSON* it = item->child; it != nullptr; it = it->next) {
        if (cJSON_GetArraySize(it) != 2) {
            continue;
        }
        ma_class_t t;
        t.score  = cJSON_GetArrayItem(it, 0)->valuedouble;
        t.target = cJSON_GetArrayItem(it, 1)->valueint;
        value.emplace_front(std::move(t));
    }
    return MA_OK;
};
ma_err_t DecoderJSON::read(std::forward_list<ma_point_t>& value) {
    value.clear();
    cJSON* item = cJSON_GetObjectItem(m_data, "points");
    if (item == nullptr || item->type != cJSON_Array) {
        return MA_FAILED;
    }
    for (cJSON* it = item->child; it != nullptr; it = it->next) {
        if (cJSON_GetArraySize(it) != 4) {
            continue;
        }
        ma_point_t t;
        t.x      = cJSON_GetArrayItem(it, 0)->valuedouble;
        t.y      = cJSON_GetArrayItem(it, 1)->valuedouble;
        t.score  = cJSON_GetArrayItem(it, 2)->valuedouble;
        t.target = cJSON_GetArrayItem(it, 3)->valueint;
        value.emplace_front(std::move(t));
    }
    return MA_OK;
};
ma_err_t DecoderJSON::read(std::forward_list<ma_bbox_t>& value) {
    value.clear();
    cJSON* item = cJSON_GetObjectItem(m_data, "boxes");
    if (item == nullptr || item->type != cJSON_Array) {
        return MA_FAILED;
    }
    for (cJSON* it = item->child; it != nullptr; it = it->next) {
        if (cJSON_GetArraySize(it) != 6) {
            continue;
        }
        ma_bbox_t t;
        t.x      = cJSON_GetArrayItem(it, 0)->valuedouble;
        t.y      = cJSON_GetArrayItem(it, 1)->valuedouble;
        t.w      = cJSON_GetArrayItem(it, 2)->valuedouble;
        t.h      = cJSON_GetArrayItem(it, 3)->valuedouble;
        t.score  = cJSON_GetArrayItem(it, 4)->valuedouble;
        t.target = cJSON_GetArrayItem(it, 5)->valueint;
        value.emplace_front(std::move(t));
    }
    return MA_OK;
};

}  // namespace ma