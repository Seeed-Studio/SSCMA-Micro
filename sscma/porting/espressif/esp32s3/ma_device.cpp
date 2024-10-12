
#include <core/ma_compiler.h>
#include <core/ma_debug.h>

#include <porting/ma_device.h>

#include <cstring>


#include "ma_camera_esp32.h"
#include "ma_config_board.h"
#include "ma_storage_lfs.h"
#include "ma_transport_console.h"

#include <esp_efuse.h>
#include <esp_efuse_chip.h>
#include <esp_efuse_table.h>
#include <hal/efuse_hal.h>
#include <esp_partition.h>
#include <spi_flash_mmap.h>


extern "C" {

void ma_invoke_pre_hook(void*) {}

void ma_invoke_post_hook(void*) {}
}

static inline uint32_t _device_id_from_efuse() {
    char id_full[16]{};
    esp_err_t err = esp_efuse_read_field_blob(ESP_EFUSE_OPTIONAL_UNIQUE_ID, id_full, 16u << 3);

    if (err != ESP_OK) [[unlikely]]
        return 0ul;

    // Fowler–Noll–Vo hash function
    uint32_t hash  = 0x811c9dc5;
    uint32_t prime = 0x1000193;
    for (size_t i = 0; i < 16; ++i) {
        uint8_t value = id_full[i];
        hash          = hash ^ value;
        hash *= prime;
    }

    return hash;
}

namespace ma {

Device::Device() {
    MA_LOGD(MA_TAG, "Initializing device: %s", MA_BOARD_NAME);
    { m_name = MA_BOARD_NAME; }

    MA_LOGD(MA_TAG, "Initializing device version");
    { m_version = "XIAO(ESP32S3)"; }

    MA_LOGD(MA_TAG, "Initializing device ID");
    { m_id = std::to_string((size_t)_device_id_from_efuse()); }

    MA_LOGD(MA_TAG, "Initializing storage");
    {
        static ma_lfs_bd_cfg_t bd_config = {
            .read_size   = 16,
            .prog_size   = 16,
            .erase_size  = 4096,
            .erase_count = 48,
            .flash_addr  = nullptr,
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
    }

    MA_LOGD(MA_TAG, "Initializing sensors");
    {
        static CameraESP32 camera(0);
        m_sensors.push_back(&camera);
    }

    MA_LOGD(MA_TAG, "Initializing models");
    {
        const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "models");
        if (partition) {
            const uint8_t** mmap = nullptr;
            uint32_t* handler    = nullptr;
            esp_err_t ret        = spi_flash_mmap(partition->address, partition->size, SPI_FLASH_MMAP_DATA, reinterpret_cast<const void**>(mmap), handler);

            if (ret != ESP_OK) {
                MA_LOGE(MA_TAG, "Failed to map models partition");
                return;
            }
            const void* address = *mmap;
            const size_t size   = partition->size;
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
    }
}

Device* Device::getInstance() {
    static Device device{};
    return &device;
}

Device::~Device() {
    // NVIC_SystemReset();
}

}  // namespace ma