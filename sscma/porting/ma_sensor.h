#ifndef _MA_SENSOR_H_
#define _MA_SENSOR_H_

#include <core/ma_types.h>

#include <cstdint>
#include <vector>

namespace ma {

enum ma_sensor_type_e {
    MA_SENSOR_TYPE_UNKNOWN    = 0,
    MA_SENSOR_TYPE_CAMERA     = 1,
    MA_SENSOR_TYPE_MICROPHONE = 2,
    MA_SENSOR_TYPE_IMU        = 3,
};

struct ma_sensor_preset_t {
    const char* description;
};

struct ma_sensor_presets_t {
    std::vector<ma_sensor_preset_t> presets;
};

class Sensor {
   public:
    Sensor(ma_sensor_type_e type) : m_type(type), m_initialized(false), m_preset_idx(0) {
        if (m_presets.presets.empty()) {
            m_presets.presets.push_back({.description = "Default"});
        }
    }
    virtual ~Sensor() = default;

    virtual ma_err_t init(size_t preset_idx) = 0;
    virtual ma_err_t deInit()                = 0;

    operator bool() const { return m_initialized; }

    const ma_sensor_presets_t& availablePresets() const { return m_presets; }
    const ma_sensor_preset_t&  currentPreset() const {
        if (m_preset_idx < m_presets.presets.size()) {
            return m_presets.presets[m_preset_idx];
        }
        return m_presets.presets[0];
    }

   protected:
    ma_sensor_type_e    m_type;
    bool                m_initialized;
    size_t              m_preset_idx;
    ma_sensor_presets_t m_presets;
};

}  // namespace ma

#endif