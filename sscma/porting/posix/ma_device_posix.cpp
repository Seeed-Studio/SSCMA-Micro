#include <random>

#include "core/ma_common.h"

#include "ma_storage_file.h"
#include "ma_transport_mqtt.h"

#include "ma_device_posix.h"

namespace ma {

DevicePosix::DevicePosix() {}

DevicePosix::~DevicePosix() {}

ma_err_t DevicePosix::init() {
    static StorageFile storage(MA_CONFIG_FILE);

    m_storage = &storage;
    m_name    = MA_BOARD_NAME;

    if (MA_OK != m_storage->get(MA_STORAGE_KEY_ID, m_id)) {
        // Create a random device and seed it
        std::random_device rd;
        std::mt19937 gen(rd());

        // Define the range
        std::uniform_int_distribution<> dis(0x10000000, 0x7FFFFFFF);

        // Generate the random number
        m_id = dis(gen);

        // Set device id
        m_storage->set(MA_STORAGE_KEY_ID, m_id);
    }
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