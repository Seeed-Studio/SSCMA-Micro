#include <random>

#include "core/ma_common.h"

#include "ma_storage_file.h"
#include "ma_transport_mqtt.h"

#include "ma_device_posix.h"

namespace ma {

DevicePosix::DevicePosix() {
    static StorageFile storage(MA_CONFIG_FILE);

    m_storage = &storage;
    m_name    = MA_BOARD_NAME;
    m_version = "v1";

    // TODO: read id from system
    // Create a random device and seed it
    std::random_device rd;
    std::mt19937 gen(rd());

    // Define the range
    std::uniform_int_distribution<> dis(0x10000000, 0x7FFFFFFF);

    m_storage->get(MA_STORAGE_KEY_ID, m_id, std::to_string(dis(gen)));
}

DevicePosix::~DevicePosix() {}

ma_err_t DevicePosix::init() {

    return MA_OK;
}

ma_err_t DevicePosix::deinit() {
    return MA_OK;
}

Device* Device::getInstance() {
    static DevicePosix Device;
    return &Device;
}

}  // namespace ma