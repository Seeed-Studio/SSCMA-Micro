#include <WE2_device_addr.h>
#include <core/ma_debug.h>
#include <hx_drv_scu.h>
#include <porting/ma_device.h>

#include <cstring>

#include "drivers/xip.h"
#include "ma_camera_himax.h"
#include "ma_config_board.h"
#include "ma_storage_lfs.h"
#include "ma_transport_console.h"
#include "ma_transport_serial.h"

namespace ma {

static const char* TAG = "ma::device";

Device::Device() {
    MA_LOGD(TAG, "Initializing device: %s", MA_BOARD_NAME);
    {
        bool success = xip_drv_init();
        MA_ASSERT(success && "Failed to initialize XIP");
        m_name = MA_BOARD_NAME;
    }

    MA_LOGD(TAG, "Initializing device version");
    {
        uint32_t chipid;
        uint32_t version;
        hx_drv_scu_get_version(&chipid, &version);
        if (chipid == 0x8538000c) {
            m_version = "HX6538";
        } else {
            m_version = "HX6539";
        }
    }

    MA_LOGD(TAG, "Initializing device ID");
    {
        xip_ownership_acquire();
        uint8_t id_full[16];
        if (xip_safe_enable()) {
            std::memcpy(id_full, reinterpret_cast<const void*>(BASE_ADDR_FLASH1_R_ALIAS + 0x003DF000), sizeof id_full);
        } else {
            MA_LOGE(TAG, "Failed to read device ID, using default");
        }
        xip_ownership_release();
        for (size_t i; i < sizeof id_full; ++i) {
            m_id += std::to_string(id_full[i]);
        }
    }

    MA_LOGD(TAG, "Initializing storage");
    {
        lfs_flashbd_config bd_config = {
          .read_size   = 16,
          .prog_size   = 16,
          .erase_size  = 4096,
          .erase_count = 128,
          .flash_addr  = reinterpret_cast<void*>(BASE_ADDR_FLASH1_R_ALIAS + 0x00400000),
        };
        static StorageLfs storage;
        storage.init(&bd_config);
        m_storage = &storage;
    }

    MA_LOGD(TAG, "Initializing transports");
    {
        static Console console;
        console.init(nullptr);
        m_transports.push_back(&console);

        static Serial serial;
        serial.init(nullptr);
        m_transports.push_back(&serial);
    }

    MA_LOGD(TAG, "Initializing sensors");
    {
        static CameraHimax camera;
        m_sensors.push_back(&camera);
    }
}

Device* Device::getInstance() {
    static Device device{};
    return &device;
}

Device::~Device() { NVIC_SystemReset(); }

}  // namespace ma