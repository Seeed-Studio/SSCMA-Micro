#pragma once

#include <el_config_porting.h>

#include <cstdint>
#include <cstring>
#include <functional>
#include <new>
#include <string>

#include "porting/el_extfs.h"

namespace sscma::extension {

using namespace edgelab;

class ContentsExport {
   public:
    // Retry count should always larger than 0
    // on WE2 it should be 1 because its middleware may panic system on the second try
    ContentsExport(int retry = 1) : _retry(retry), _extfs(nullptr), _path(""), _bytes(0), _size(0), _data(nullptr) {}

    ~ContentsExport() {
        if (_data != nullptr) {
            delete[] _data;
            _data = nullptr;
        }
    };

    bool cache(const uint8_t* data, size_t size, size_t realloc_trigger = 1024 * 10) {
        if (_data == nullptr) [[unlikely]] {
            _data = new (std::nothrow) uint8_t[size + (realloc_trigger >> 1)];
            if (_data == nullptr) {
                return false;
            }
            _size = size + (realloc_trigger >> 1);
        } else if (size > _size || size + realloc_trigger < _size) {
            delete[] _data;
            uint8_t* _data = new (std::nothrow) uint8_t[size + (realloc_trigger >> 1)];
            if (_data == nullptr) {
                return false;
            }
            _size = size + (realloc_trigger >> 1);
        }

        _bytes = size;
        std::memcpy(_data, data, _bytes);

        return true;
    }

    bool commit(std::string name, std::function<void(Status)> callback) {
        if (!init_filesystem(callback)) {
            _extfs = nullptr;
            return false;
        }

        std::string file    = _path + name;
        auto        handler = _extfs->open(file.c_str(), OpenMode::WRITE);
        if (!handler.status.success) {
            callback(handler.status);
            return false;
        }

        size_t written = 0;
        auto   status  = handler.file->write(_data, _bytes, &written);
        if (!status.success) {
            callback(status);
            return false;
        }

        handler.file->close();

        return true;
    }

   protected:
    bool init_filesystem(std::function<void(Status)>& callback) {
        if (_extfs != nullptr) [[unlikely]] {
            return true;
        }

        if (_retry > 0) {
            --_retry;
        } else if (_retry <= 0) {
            callback({false, "Error, init filesystem count exceeded"});
            return false;
        }

        _extfs = Extfs::get_ptr();
        if (_extfs == nullptr) {
            callback({false, "Device does not support filesystem"});
            return false;
        }

        auto res = _extfs->mount("/");
        if (!res.success) {
            callback(res);
            return false;
        }

        _path = PORT_DEVICE_NAME;
        _path += " Export";

        if (!_extfs->exists(_path.c_str())) {
            res = _extfs->mkdir(_path.c_str());
            if (!res.success) {
                callback(res);
                return false;
            }
        }

        std::string info_file = _path + "/.sscma";
        std::size_t dir_id    = 0;
        if (!_extfs->exists(info_file.c_str())) {
            auto handler = _extfs->open(info_file.c_str(), OpenMode::WRITE);
            if (!handler.status.success) {
                callback(handler.status);
                return false;
            }
            std::string contents = std::to_string(dir_id);
            handler.file->write(reinterpret_cast<const uint8_t*>(contents.c_str()), contents.size(), nullptr);
            handler.file->close();
        } else {
            auto handler = _extfs->open(info_file.c_str(), OpenMode::READ);
            if (!handler.status.success) {
                callback(handler.status);
                return false;
            }
            uint8_t buffer[64]{};
            size_t  read = 0;
            handler.file->read(buffer, sizeof(buffer), &read);
            buffer[read] = '\0';
            dir_id       = std::atoi(reinterpret_cast<const char*>(buffer)) + 1;
            handler.file->close();

            handler = _extfs->open(info_file.c_str(), OpenMode::WRITE);
            if (!handler.status.success) {
                callback(handler.status);
                return false;
            }
            int len = std::snprintf(reinterpret_cast<char*>(buffer), sizeof(buffer), "%ld", dir_id);
            res     = handler.file->write(buffer, len > 0 ? len : 0, nullptr);
            if (!res.success) {
                callback(res);
                return false;
            }

            handler.file->close();
        }

        _path += '/';
        _path += std::to_string(dir_id);

        res = _extfs->mkdir(_path.c_str());
        if (!res.success) {
            callback(res);
            return false;
        }

        _path += '/';

        return true;
    }

   private:
    int _retry;

    Extfs* _extfs;

    std::string _path;

    size_t _bytes;

    size_t   _size;
    uint8_t* _data;
};

}  // namespace sscma::extension
