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

using ma_sensor_presets_t = std::vector<ma_sensor_preset_t>;

class Sensor {
   public:
    explicit Sensor(ma_sensor_type_e type) noexcept : m_type(type), m_initialized(false), m_preset_idx(0) {
        if (m_presets.empty()) {
            m_presets.push_back({.description = "Default"});
        }
    }
    virtual ~Sensor() = default;

    [[nodiscard]] virtual ma_err_t init(size_t preset_idx) noexcept = 0;
    virtual void                   deInit() noexcept                = 0;

    [[nodiscard]] operator bool() const noexcept { return m_initialized; }
    [[nodiscard]] ma_sensor_type_e getType() const noexcept { return m_type; }

    const ma_sensor_presets_t& availablePresets() const noexcept { return m_presets; }
    const ma_sensor_preset_t&  currentPreset() const noexcept {
        if (m_preset_idx < m_presets.size()) {
            return m_presets[m_preset_idx];
        }
        return m_presets[0];
    }

   protected:
    const ma_sensor_type_e m_type;
    bool                   m_initialized;
    size_t                 m_preset_idx;
    ma_sensor_presets_t    m_presets;
};

}  // namespace ma

#endif