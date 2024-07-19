#include <cJSON.h>

#include "ma_codec_json.h"

namespace ma {

static const char* TAG = "ma::codec::JSON";

EncoderJSON::EncoderJSON() : m_mutex(false), m_root(nullptr), m_data(nullptr) {
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

    if (cJSON_AddNumberToObject(m_root, "type", type) == nullptr ||
        cJSON_AddStringToObject(m_root, "name", name.c_str()) == nullptr ||
        cJSON_AddNumberToObject(m_root, "code", code) == nullptr) [[unlikely]] {
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

ma_err_t EncoderJSON::begin(ma_msg_type_t type,
                            ma_err_t code,
                            const std::string& name,
                            const std::string& data) {
    m_mutex.lock();
    ma_err_t ret = reset();
    if (ret != MA_OK) [[unlikely]] {
        m_mutex.unlock();
        return ret;
    }
    m_root = cJSON_CreateObject();
    if (cJSON_AddNumberToObject(m_root, "type", type) == nullptr ||
        cJSON_AddStringToObject(m_root, "name", name.c_str()) == nullptr ||
        cJSON_AddNumberToObject(m_root, "code", code) == nullptr ||
        cJSON_AddStringToObject(m_root, "data", data.c_str()) == nullptr) [[unlikely]] {
        reset();
        m_mutex.unlock();
        return MA_FAILED;
    }
    return MA_OK;
}

ma_err_t EncoderJSON::begin(ma_msg_type_t type,
                            ma_err_t code,
                            const std::string& name,
                            uint64_t data) {
    m_mutex.lock();
    ma_err_t ret = reset();
    if (ret != MA_OK) [[unlikely]] {
        m_mutex.unlock();
        return ret;
    }
    m_root = cJSON_CreateObject();
    if (cJSON_AddNumberToObject(m_root, "type", type) == nullptr ||
        cJSON_AddStringToObject(m_root, "name", name.c_str()) == nullptr ||
        cJSON_AddNumberToObject(m_root, "code", code) == nullptr ||
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
    return cJSON_AddStringToObject(m_data, key.c_str(), value.c_str()) != nullptr ? MA_OK
                                                                                  : MA_FAILED;
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
    cJSON_AddItemToObject(item, "name", cJSON_CreateString(static_cast<char*>(value.name)));
    cJSON_AddItemToObject(item, "address", cJSON_CreateString(static_cast<char*>(value.addr)));
#else
    cJSON_AddItemToObject(item, "name", cJSON_CreateNumber(static_cast<int>(value.name)));
    cJSON_AddItemToObject(item, "address", cJSON_CreateNumber(static_cast<int>(value.addr)));
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

ma_err_t EncoderJSON::write(std::vector<ma_class_t>& value) {
    if (cJSON_GetObjectItem(m_data, "classes") != nullptr) {
        return MA_EEXIST;
    }
    cJSON* array = cJSON_AddArrayToObject(m_data, "classes");
    if (array == nullptr) {
        MA_LOGD(TAG, "1cJSON_AddArrayToObject failed");
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

ma_err_t EncoderJSON::write(std::vector<ma_point_t>& value) {
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
ma_err_t EncoderJSON::write(std::vector<ma_bbox_t>& value) {
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

ma_err_t EncoderJSON::write(std::vector<ma_model_t>& value) {
    cJSON* array = cJSON_CreateArray();
    cJSON_ReplaceItemInObjectCaseSensitive(m_root, "data", array);
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
        cJSON_AddItemToObject(item, "name", cJSON_CreateString(static_cast<char*>(it->name)));
        cJSON_AddItemToObject(item, "address", cJSON_CreateString(static_cast<char*>(it->addr)));
#else
        cJSON_AddItemToObject(item, "name", cJSON_CreateNumber(static_cast<int>(it->name)));
        cJSON_AddItemToObject(item, "address", cJSON_CreateNumber(static_cast<int>(it->addr)));
#endif

        cJSON_AddItemToArray(array, item);
    }
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
ma_err_t DecoderJSON::read(std::vector<ma_class_t>& value) {
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
        value.push_back(std::move(t));
    }
    return MA_OK;
};
ma_err_t DecoderJSON::read(std::vector<ma_point_t>& value) {
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
        value.push_back(std::move(t));
    }
    return MA_OK;
};
ma_err_t DecoderJSON::read(std::vector<ma_bbox_t>& value) {
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
        value.push_back(std::move(t));
    }
    return MA_OK;
};


}  // namespace ma