#pragma once

#include <el_config_porting.h>

#include <cstdint>
#include <cstring>
#include <functional>
#include <new>
#include <string>

#include "porting/el_extfs.h"

#ifndef MA_CONTENT_EXPORT_CACHE_ADDR
    #define MA_CONTENT_EXPORT_CACHE_DISABLED 1
    #define MA_CONTENT_EXPORT_CACHE_ADDR     0
#endif

#ifndef MA_CONTENT_EXPORT_CACHE_SIZE
    #define MA_CONTENT_EXPORT_CACHE_SIZE 0
#endif

namespace sscma::extension {

using namespace edgelab;

class ContentsExport {
   public:
    ContentsExport()
        : _extfs(nullptr),
          _path(""),
          _bytes(0),
          _size(MA_CONTENT_EXPORT_CACHE_SIZE),
          _data(MA_CONTENT_EXPORT_CACHE_ADDR) {}

    ~ContentsExport() {
#if MA_CONTENT_EXPORT_CACHE_DISABLED
        if (_data != nullptr) {
            delete[] _data;
            _data = nullptr;
        }
#endif
    };

    bool cache(const uint8_t* data, size_t size, size_t realloc_trigger = 1024 * 10) {
#ifdef MA_CONTENT_EXPORT_CACHE_DISABLED
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
#else
        _bytes = size > _size ? _size : size;
#endif
        std::memcpy(_data, data, _bytes);

        return true;
    }

    bool cache_result(std::string&& result_str) {
        _result = std::move(result_str);
        return true;
    }

    bool commit(std::string name, std::function<void(Status)> callback) {
        if (_data == nullptr || _bytes <= 0) [[unlikely]] {
            return false;
        }

        if (!init_filesystem(callback)) [[unlikely]] {
            _extfs = nullptr;
            return false;
        }

        const auto& file_prefix = name;
        {
            std::string file    = file_prefix + ".jpeg";
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
            _bytes = 0;
            handler.file->close();
        }

        if (_result.size()) {
            std::string file = file_prefix + ".json";
            auto handler = _extfs->open(file.c_str(), OpenMode::WRITE);
            if (!handler.status.success) {
                return false;
            }

            size_t written = 0;
            auto   status =
              handler.file->write(reinterpret_cast<const uint8_t*>(_result.c_str()), _result.size(), &written);
            if (!status.success) {
                return false;
            }

            _result.clear();
            _result.shrink_to_fit();

            handler.file->close();
        }

        return true;
    }

   protected:
    bool init_filesystem(std::function<void(Status)>& callback) {
        if (_extfs != nullptr) [[unlikely]] {
            return true;
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
            int len = std::snprintf(reinterpret_cast<char*>(buffer), sizeof(buffer), "%d", dir_id);
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

        res = _extfs->cd(_path.c_str());
        if (!res.success) {
            callback(res);
            return false;
        }

        return true;
    }

   private:
    Extfs* _extfs;

    std::string _path;

    size_t _bytes;

    size_t   _size;
    uint8_t* _data;

    std::string _result;
};

}  // namespace sscma::extension
