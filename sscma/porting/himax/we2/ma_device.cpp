#include <WE2_device_addr.h>
#include <core/ma_compiler.h>
#include <core/ma_debug.h>
#include <hx_drv_scu.h>
#include <porting/ma_device.h>

#include <cstring>

#include "drivers/npu.h"
#include "drivers/xip.h"
#include "ma_camera_himax.h"
#include "ma_config_board.h"
#include "ma_storage_lfs.h"
#include "ma_transport_console.h"
#include "ma_transport_i2c.h"
#include "ma_transport_serial.h"

namespace ma {

Device::Device() {
    MA_LOGD(MA_TAG, "Initializing device: %s", MA_BOARD_NAME);
    {
        bool success = xip_drv_init();
        MA_ASSERT(success && "Failed to initialize XIP");
        m_name = MA_BOARD_NAME;
    }

    MA_LOGD(MA_TAG, "Initializing device version");
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

    MA_LOGD(MA_TAG, "Initializing device ID");
    {
        xip_ownership_acquire();
        uint8_t id_full[16];
        if (xip_safe_enable()) {
            std::memcpy(id_full, reinterpret_cast<const void*>(BASE_ADDR_FLASH1_R_ALIAS + 0x003DF000), sizeof id_full);
        } else {
            MA_LOGE(MA_TAG, "Failed to read device ID, using default");
        }
        xip_ownership_release();
        for (size_t i; i < sizeof id_full; ++i) {
            m_id += std::to_string(id_full[i]);
        }
    }

    MA_LOGD(MA_TAG, "Initializing storage");
    {
        static ma_lfs_bd_cfg_t bd_config = {
            .read_size   = 16,
            .prog_size   = 16,
            .erase_size  = 4096,
            .erase_count = 128,
            .flash_addr  = reinterpret_cast<void*>(BASE_ADDR_FLASH1_R_ALIAS + 0x00300000),
        };
        static StorageLfs storage;
        storage.init(&bd_config);
        m_storage = &storage;

        {
            MA_STORAGE_GET_POD(m_storage, "ma#bootcount", m_bootcount, 0);
            ++m_bootcount;
            MA_STORAGE_NOSTA_SET_POD(m_storage, "ma#bootcount", m_bootcount);
        }
    }

    MA_LOGD(MA_TAG, "Initializing transports");
    {
        static Console console;
        console.init(nullptr);
        m_transports.push_back(&console);

        static Serial serial;
        serial.init(nullptr);
        m_transports.push_back(&serial);

#ifdef MA_CONFIG_BOARD_I2C_SLAVE
        static I2C i2c;
        i2c.init(nullptr);
        m_transports.push_back(&i2c);
#endif
    }

    MA_LOGD(MA_TAG, "Initializing sensors");
    {
        static CameraHimax camera(0);
        m_sensors.push_back(&camera);
    }

    MA_LOGD(MA_TAG, "Initializing models");
    {
        xip_ownership_acquire();
        if (xip_safe_enable()) {
            const void* address = reinterpret_cast<void*>(BASE_ADDR_FLASH1_R_ALIAS + 0x00400000);
            const size_t size   = 4096;
            size_t id           = 0;
            for (size_t i = 0; i < size; i += 4096) {
                const void* data = address + i;
                if (ma_ntohl(*(static_cast<const uint32_t*>(data) + 1)) == 0x54464C33) {  // 'TFL3'
                    ma_model_t model;
                    model.id   = ++id;
                    model.type = MA_MODEL_TYPE_UNDEFINED;
                    model.name = address;
                    model.addr = data;
                    m_models.push_back(model);
                }
            }
        } else {
            MA_LOGE(MA_TAG, "Failed to find models, using default");
        }
        xip_ownership_release();
    }

    MA_LOGD(MA_TAG, "Initializing NPU driver");
    { himax_arm_npu_init(true, true); }
}

Device* Device::getInstance() {
    static Device device{};
    return &device;
}

Device::~Device() {
    NVIC_SystemReset();
}

}  // namespace ma