#include <cJSON.h>

#include "ma_codec_json.h"

namespace ma {

static const char* TAG = "ma::codec::JSON";

CodecJSON::CodecJSON() : m_mutex(false), m_root(nullptr), m_data(nullptr) {
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

CodecJSON::~CodecJSON() {
    if (m_root) {
        cJSON_Delete(m_root);
    }
}

CodecJSON::operator bool() const {
    return m_root != nullptr;
}

ma_err_t CodecJSON::begin() {
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

ma_err_t CodecJSON::begin(ma_reply_t reply, ma_err_t code, const std::string& name) {
    m_mutex.lock();
    ma_err_t ret = reset();

    if (ret != MA_OK) [[unlikely]] {
        goto err;
    }

    m_root = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(m_root, "type", reply) == nullptr ||
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

ma_err_t CodecJSON::begin(ma_reply_t reply,
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
    if (cJSON_AddNumberToObject(m_root, "type", reply) == nullptr ||
        cJSON_AddStringToObject(m_root, "name", name.c_str()) == nullptr ||
        cJSON_AddNumberToObject(m_root, "code", code) == nullptr ||
        cJSON_AddStringToObject(m_root, "data", data.c_str()) == nullptr) [[unlikely]] {
        reset();
        m_mutex.unlock();
        return MA_FAILED;
    }
    return MA_OK;
}

ma_err_t CodecJSON::begin(ma_reply_t reply, ma_err_t code, const std::string& name, uint64_t data) {
    m_mutex.lock();
    ma_err_t ret = reset();
    if (ret != MA_OK) [[unlikely]] {
        m_mutex.unlock();
        return ret;
    }
    m_root = cJSON_CreateObject();
    if (cJSON_AddNumberToObject(m_root, "type", reply) == nullptr ||
        cJSON_AddStringToObject(m_root, "name", name.c_str()) == nullptr ||
        cJSON_AddNumberToObject(m_root, "code", code) == nullptr ||
        cJSON_AddNumberToObject(m_root, "data", data) == nullptr) [[unlikely]] {
        reset();
        m_mutex.unlock();
        return MA_FAILED;
    }
    return MA_OK;
}

ma_err_t CodecJSON::end() {
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

const std::string& CodecJSON::toString() const {
    return m_string;
}

const void* CodecJSON::data() const {
    return m_string.c_str();
}

const size_t CodecJSON::size() const {
    return m_string.size();
}

ma_err_t CodecJSON::reset() {
    m_string.clear();
    m_string.shrink_to_fit();
    if (m_root) {
        cJSON_Delete(m_root);
        m_root = nullptr;
    }
    return MA_OK;
}

ma_err_t CodecJSON::remove(const std::string& key) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) == nullptr) {
        return MA_ENOENT;
    }
    cJSON_DeleteItemFromObject(m_data, key.c_str());
    return MA_OK;
}

ma_err_t CodecJSON::write(const std::string& key, int8_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, int16_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, int32_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, int64_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, uint8_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, uint16_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, uint32_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, uint64_t value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, float value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, double value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddNumberToObject(m_data, key.c_str(), value) != nullptr ? MA_OK : MA_FAILED;
}

ma_err_t CodecJSON::write(const std::string& key, const std::string& value) {
    if (cJSON_GetObjectItem(m_data, key.c_str()) != nullptr) {
        return MA_EEXIST;
    }
    return cJSON_AddStringToObject(m_data, key.c_str(), value.c_str()) != nullptr ? MA_OK
                                                                                  : MA_FAILED;
}

ma_err_t CodecJSON::write(ma_perf_t value) {
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

ma_err_t CodecJSON::write(std::vector<ma_class_t>& value) {
    if (cJSON_GetObjectItem(m_data, "classes") != nullptr) {
        return MA_EEXIST;
    }
    cJSON* array = cJSON_AddArrayToObject(m_data, "classes");
    if (array == nullptr) {
        return MA_FAILED;
    }
    for (auto it = value.begin(); it != value.end(); it++) {
        cJSON* item = cJSON_AddArrayToObject(array, nullptr);
        if (item == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->score));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->target));
    }
    return MA_OK;
}

ma_err_t CodecJSON::write(std::vector<ma_point_t>& value) {
    if (cJSON_GetObjectItem(m_data, "points") != nullptr) {
        return MA_EEXIST;
    }
    cJSON* array = cJSON_AddArrayToObject(m_data, "points");
    if (array == nullptr) {
        return MA_FAILED;
    }
    for (auto it = value.begin(); it != value.end(); it++) {
        cJSON* item = cJSON_AddArrayToObject(array, nullptr);
        if (item == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->x));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->y));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->score));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->target));
    }
    return MA_OK;
}
ma_err_t CodecJSON::write(std::vector<ma_bbox_t>& value) {
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
        cJSON* item = cJSON_AddArrayToObject(array, nullptr);
        if (item == nullptr) {
            return MA_FAILED;
        }
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->x));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->y));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->w));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->h));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->score));
        cJSON_AddItemToArray(item, cJSON_CreateNumber(it->target));
    }
    return MA_OK;
}


}  // namespace ma