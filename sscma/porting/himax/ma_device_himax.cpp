#include "ma_device_himax.h"

#include <random>

#include "core/ma_common.h"
#include "ma_storage_lfs.h"
#include "ma_transport_console.h"

namespace ma {

DeviceHimax::DeviceHimax() {
    static StorageLfs storage(256 * 1024);

    m_storage = &storage;
    m_name    = MA_BOARD_NAME;
    m_version = "v1";

    static Console console;
    console.open();

    m_transports.push_front(&console);


    // TODO: read id from system
    // Create a random device and seed it
    std::random_device rd;
    std::mt19937       gen(rd());

    // Define the range
    std::uniform_int_distribution<> dis(0x10000000, 0x7FFFFFFF);
    m_id = std::to_string(dis(gen));
}

DeviceHimax::~DeviceHimax() {}

ma_err_t DeviceHimax::init() { return MA_OK; }

ma_err_t DeviceHimax::deinit() { return MA_OK; }

Device* Device::getInstance() {
    static DeviceHimax Device;
    return &Device;
}

}  // namespace ma