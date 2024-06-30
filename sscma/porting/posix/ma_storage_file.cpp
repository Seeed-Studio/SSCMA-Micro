#include <fstream>
#include <sstream>
#include <vector>

#include <cJSON.h>

#include "ma_storage_file.h"

namespace ma {

const static char* TAG = "ma::storage::file";

const std::string default_filename = "sscma.conf";

StorageFile::StorageFile(const std::string& filename) : filename_(filename), root_(nullptr) {
    load();
}

StorageFile::StorageFile() : filename_(default_filename), root_(nullptr) {
    load();
}

StorageFile::~StorageFile() {
    save();
    if (root_) {
        cJSON_Delete(root_);
    }
}

void StorageFile::save() {
    Guard guard(mutex_);
    if (root_) {
        char* jsonString = cJSON_Print(root_);
        std::ofstream file(filename_);
        file << jsonString;
        file.close();
        cJSON_free(jsonString);
    }
}

void StorageFile::load() {
    Guard guard(mutex_);
    std::ifstream file(filename_);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        root_ = cJSON_Parse(buffer.str().c_str());
    }
    if (!root_) {
        root_ = cJSON_CreateObject();
    }
}

std::vector<std::string> splitKey(const std::string& key) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end   = key.find('#');
    while (end != std::string::npos) {
        parts.push_back(key.substr(start, end - start));
        start = end + 1;
        end   = key.find('#', start);
    }
    parts.push_back(key.substr(start));
    return parts;
}

cJSON* StorageFile::getNode(const std::string& key) {
    Guard guard(mutex_);
    std::vector<std::string> parts = splitKey(key);
    cJSON* current                 = root_;
    for (const auto& part : parts) {
        current = cJSON_GetObjectItemCaseSensitive(current, part.c_str());
        if (!current) {
            return nullptr;
        }
    }
    return current;
}

void StorageFile::setNode(const std::string& key, cJSON* value) {
    Guard guard(mutex_);
    std::vector<std::string> parts = splitKey(key);
    cJSON* parent                  = root_;
    for (const auto& part : parts) {
        cJSON* child = cJSON_GetObjectItemCaseSensitive(parent, part.c_str());
        if (!child) {
            child = cJSON_CreateObject();
            cJSON_AddItemToObject(parent, part.c_str(), child);
        }
        if (part == parts.back()) {
            cJSON_ReplaceItemInObjectCaseSensitive(parent, part.c_str(), value);
            break;
        }
        parent = child;
    }
    save();
}

ma_err_t StorageFile::set(const std::string& key, bool value) {
    setNode(key, cJSON_CreateBool(value));
    return MA_OK;
}

ma_err_t StorageFile::set(const std::string& key, int8_t value) {
    return set(key, static_cast<int64_t>(value));
}

ma_err_t StorageFile::set(const std::string& key, int16_t value) {
    return set(key, static_cast<int64_t>(value));
}

ma_err_t StorageFile::set(const std::string& key, int32_t value) {
    return set(key, static_cast<int64_t>(value));
}

ma_err_t StorageFile::set(const std::string& key, int64_t value) {
    setNode(key, cJSON_CreateNumber(static_cast<double>(value)));
    return MA_OK;
}

ma_err_t StorageFile::set(const std::string& key, uint8_t value) {
    return set(key, static_cast<uint64_t>(value));
}

ma_err_t StorageFile::set(const std::string& key, uint16_t value) {
    return set(key, static_cast<uint64_t>(value));
}

ma_err_t StorageFile::set(const std::string& key, uint32_t value) {
    return set(key, static_cast<uint64_t>(value));
}

ma_err_t StorageFile::set(const std::string& key, uint64_t value) {
    setNode(key, cJSON_CreateNumber(static_cast<double>(value)));
    return MA_OK;
}

ma_err_t StorageFile::set(const std::string& key, float value) {
    setNode(key, cJSON_CreateNumber(static_cast<double>(value)));
    return MA_OK;
}

ma_err_t StorageFile::set(const std::string& key, double value) {
    setNode(key, cJSON_CreateNumber(value));
    return MA_OK;
}

ma_err_t StorageFile::set(const std::string& key, const std::string& value) {
    setNode(key, cJSON_CreateString(value.c_str()));
    return MA_OK;
}

ma_err_t StorageFile::set(const std::string& key, void* value, size_t size) {
    setNode(key, cJSON_CreateString(reinterpret_cast<char*>(value)));
    return MA_OK;
}

ma_err_t StorageFile::get(const std::string& key, bool& value) {
    cJSON* item = getNode(key);
    if (item && cJSON_IsBool(item)) {
        value = cJSON_IsTrue(item);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, int8_t& value) {
    int64_t temp;
    if (get(key, temp) == MA_OK) {
        value = static_cast<int8_t>(temp);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, int16_t& value) {
    int64_t temp;
    if (get(key, temp) == MA_OK) {
        value = static_cast<int16_t>(temp);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, int32_t& value) {
    int64_t temp;
    if (get(key, temp) == MA_OK) {
        value = static_cast<int32_t>(temp);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, int64_t& value) {
    cJSON* item = getNode(key);
    if (item && cJSON_IsNumber(item)) {
        value = static_cast<int64_t>(item->valuedouble);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, uint8_t& value) {
    uint64_t temp;
    if (get(key, temp) == MA_OK) {
        value = static_cast<uint8_t>(temp);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, uint16_t& value) {
    uint64_t temp;
    if (get(key, temp) == MA_OK) {
        value = static_cast<uint16_t>(temp);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, uint32_t& value) {
    uint64_t temp;
    if (get(key, temp) == MA_OK) {
        value = static_cast<uint32_t>(temp);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, uint64_t& value) {
    cJSON* item = getNode(key);
    if (item && cJSON_IsNumber(item)) {
        value = static_cast<uint64_t>(item->valuedouble);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, float& value) {
    cJSON* item = getNode(key);
    if (item && cJSON_IsNumber(item)) {
        value = static_cast<float>(item->valuedouble);
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, double& value) {
    cJSON* item = getNode(key);
    if (item && cJSON_IsNumber(item)) {
        value = item->valuedouble;
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::get(const std::string& key, std::string& value) {
    cJSON* item = getNode(key);
    if (item && cJSON_IsString(item)) {
        value = item->valuestring;
        return MA_OK;
    }
    return MA_ENOENT;
}

ma_err_t StorageFile::remove(const std::string& key) {
    Guard guard(mutex_);
    std::vector<std::string> parts = splitKey(key);
    cJSON* current                 = root_;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        current = cJSON_GetObjectItemCaseSensitive(current, parts[i].c_str());
        if (!current) {
            return MA_ENOENT;
        }
    }
    cJSON_DeleteItemFromObjectCaseSensitive(current, parts.back().c_str());
    save();
    return MA_OK;
}

}  // namespace ma
