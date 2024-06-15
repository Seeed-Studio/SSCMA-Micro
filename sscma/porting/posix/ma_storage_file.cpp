#include <fstream>
#include <sstream>

#include "core/ma_common.h"
#include "ma_storage_file.h"

namespace ma {

StorageFile::StorageFile(const std::string& filename) : filename_(filename) {}

StorageFile::~StorageFile() = default;

ma_err_t StorageFile::set(const std::string& key, bool value) {
    return set_impl(key, value ? "1" : "0");
}

ma_err_t StorageFile::set(const std::string& key, int8_t value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, int16_t value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, int32_t value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, int64_t value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, uint8_t value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, uint16_t value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, uint32_t value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, uint64_t value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, float value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, double value) {
    return set_impl(key, std::to_string(value));
}

ma_err_t StorageFile::set(const std::string& key, const std::string& value) {
    return set_impl(key, value);
}

ma_err_t StorageFile::set(const std::string& key, void* value, size_t size) {
    std::string str(static_cast<char*>(value), size);
    return set_impl(key, str);
}

ma_err_t StorageFile::get(const std::string& key, bool& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = (str_value == "1");
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, int8_t& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<int8_t>(std::stoi(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, int16_t& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<int16_t>(std::stoi(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, int32_t& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<int32_t>(std::stoi(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, int64_t& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<int64_t>(std::stoll(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, uint8_t& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<uint8_t>(std::stoul(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, uint16_t& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<uint16_t>(std::stoul(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, uint32_t& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<uint32_t>(std::stoul(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, uint64_t& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<uint64_t>(std::stoull(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, float& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<float>(std::stof(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, double& value) {
    std::string str_value;
    ma_err_t err = get_impl(key, str_value);
    if (err == MA_OK) {
        value = static_cast<double>(std::stod(str_value));
    }
    return err;
}

ma_err_t StorageFile::get(const std::string& key, std::string& value) {
    return get_impl(key, value);
}

ma_err_t StorageFile::remove(const std::string& key) {
    std::ifstream file_in(filename_);
    std::ofstream file_out("temp.txt");
    std::string line;
    bool found = false;

    while (std::getline(file_in, line)) {
        std::istringstream is_line(line);
        std::string file_key;
        if (std::getline(is_line, file_key, '=')) {
            if (file_key != key) {
                file_out << line << "\n";
            } else {
                found = true;
            }
        }
    }

    file_in.close();
    file_out.close();

    std::remove(filename_.c_str());
    std::rename("temp.txt", filename_.c_str());

    return found ? MA_OK : MA_ENOENT;
}

ma_err_t StorageFile::set_impl(const std::string& key, const std::string& value) {
    std::ifstream file_in(filename_);
    std::ofstream file_out("temp.txt");
    std::string line;
    bool found = false;

    while (std::getline(file_in, line)) {
        std::istringstream is_line(line);
        std::string file_key;
        if (std::getline(is_line, file_key, '=')) {
            if (file_key == key) {
                file_out << key << "=" << value << "\n";
                found = true;
            } else {
                file_out << line << "\n";
            }
        }
    }

    if (!found) {
        file_out << key << "=" << value << "\n";
    }

    file_in.close();
    file_out.close();

    std::remove(filename_.c_str());
    std::rename("temp.txt", filename_.c_str());

    return MA_OK;
}

ma_err_t StorageFile::get_impl(const std::string& key, std::string& value) {
    std::ifstream file(filename_);
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream is_line(line);
        std::string file_key;
        if (std::getline(is_line, file_key, '=')) {
            if (file_key == key) {
                std::getline(is_line, value);
                return MA_OK;
            }
        }
    }

    return MA_ENOENT;
}

}  // namespace ma
