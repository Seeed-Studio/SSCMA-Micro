#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <vector>

#include <cJSON.h>

#include "ma_storage_file.h"

namespace ma {

constexpr char TAG[] = "ma::storage::file";

StorageFile::StorageFile() : Storage(), root_(nullptr), mutex_() {}
StorageFile::~StorageFile() {
    deInit();
}

ma_err_t StorageFile::init(const void* config) noexcept {
    Guard guard(mutex_);
    if (m_initialized) [[unlikely]] {
        return MA_OK;
    }
    if (!config) [[unlikely]] {
        return MA_EINVAL;
    }
    filename_ = std::string(reinterpret_cast<const char*>(config));
    if (access(filename_.c_str(), F_OK) != 0) {
        std::ofstream file(filename_);
        file << "{}";
        file.close();
    }
    if (access(filename_.c_str(), R_OK | W_OK) != 0) {
        MA_LOGE(TAG, "Failed to access file %s", filename_.c_str());
        return MA_EINVAL;
    }
    MA_LOGD(TAG, "StorageFile::init: %s", filename_.c_str());
    load();
    m_initialized = true;
    return MA_OK;
}

void StorageFile::deInit() noexcept {
    Guard guard(mutex_);
    if (!m_initialized) [[unlikely]] {
        return;
    }
    save();
    if (root_) {
        cJSON_Delete(root_);
    }
    m_initialized = false;
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
        current = cJSON_GetObjectItem(current, part.c_str());
        if (!current) {
            return nullptr;
        }
    }
    return current;
}

ma_err_t StorageFile::setNode(const std::string& key, cJSON* value) {
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
    return MA_OK;
}

ma_err_t StorageFile::set(const std::string& key, const void* value, size_t size) noexcept {
    Guard guard(mutex_);

    if (!value) [[unlikely]] {
        return MA_EINVAL;
    }

    setNode(key, cJSON_CreateString(static_cast<const char*>(value)));

    return MA_OK;
}

ma_err_t StorageFile::get(const std::string& key, std::string& value) noexcept {
    Guard guard(mutex_);
    cJSON* node = getNode(key);
    if (!node) {
        return MA_ENOENT;
    }
    value = node->valuestring;
    return MA_OK;
}

ma_err_t StorageFile::set(const std::string& key, int64_t value) noexcept {
    return setNode(key, cJSON_CreateNumber(static_cast<double>(value)));
}

ma_err_t StorageFile::set(const std::string& key, double value) noexcept {
    return setNode(key, cJSON_CreateNumber(value));
}

ma_err_t StorageFile::get(const std::string& key, int64_t& value) noexcept {
    Guard guard(mutex_);
    cJSON* node = getNode(key);
    if (!node) {
        return MA_ENOENT;
    }
    value = static_cast<int64_t>(node->valueint);
    return MA_OK;
}

ma_err_t StorageFile::get(const std::string& key, double& value) noexcept {
    Guard guard(mutex_);
    cJSON* node = getNode(key);
    if (!node) {
        return MA_ENOENT;
    }
    value = node->valuedouble;
    return MA_OK;
}

ma_err_t StorageFile::remove(const std::string& key) noexcept {
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

bool StorageFile::exists(const std::string& key) noexcept {
    Guard guard(mutex_);
    std::vector<std::string> parts = splitKey(key);
    cJSON* current                 = root_;
    for (size_t i = 0; i < parts.size(); ++i) {
        current = cJSON_GetObjectItemCaseSensitive(current, parts[i].c_str());
        if (!current) {
            return false;
        }
    }
    return true;
}

}  // namespace ma
