#include <WE2_device_addr.h>
#include <hx_drv_scu.h>

#include <cstring>

#include "ma_device.h"
#include "ma_storage_lfs.h"
#include "ma_transport_console.h"
#include "xip.h"

namespace ma {

Device::Device() {
    m_name = MA_BOARD_NAME;

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

    {
        xip_ownership_acquire();
        uint8_t id_full[16];
        if (xip_safe_enable()) {
            std::memcpy(
              id_full, reinterpret_cast<const uint8_t*>(BASE_ADDR_FLASH1_R_ALIAS + 0x003DF000), sizeof id_full);
        }
        for (size_t i; i < sizeof id_full; ++i) {
            m_id += std::to_string(id_full[i]);
        }
    }

    {
        static StorageLfs storage(256 * 1024);
        m_storage = &storage;
    }

    {
        static Console console;
        console.open();
        m_transports.push_front(&console);
    }
}

Device* Device::getInstance() {
    static Device device{};
    return &device;
}

Device::~Device() { NVIC_SystemReset(); }

}  // namespace ma