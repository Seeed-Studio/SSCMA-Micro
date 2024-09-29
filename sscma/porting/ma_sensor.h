#ifndef _MA_SENSOR_H_
#define _MA_SENSOR_H_

#include <core/ma_types.h>

#include <cstdint>
#include <string>
#include <vector>

namespace ma {

class Sensor {
   public:
    enum class Type : int {
        kUnknown    = 0,
        kCamera     = 1,
        kMicrophone = 2,
        kIMU        = 3,
    };

    static std::string __repr__(Type type) noexcept {
        switch (type) {
        case Type::kCamera:
            return "Camera";
        case Type::kMicrophone:
            return "Microphone";
        case Type::kIMU:
            return "IMU";
        default:
            return "Unknown";
        }
    }

    struct Preset {
        const char* description = nullptr;
    };

    using Presets = std::vector<Preset>;

   public:
    explicit Sensor(size_t id, Type type) noexcept : m_id(id), m_type(type), m_initialized(false), m_preset_idx(0) {
        if (m_presets.empty()) {
            m_presets.push_back({.description = "Default"});
        }
    }
    virtual ~Sensor() = default;

    [[nodiscard]] virtual ma_err_t init(size_t preset_idx) noexcept = 0;
    virtual void                   deInit() noexcept                = 0;

    [[nodiscard]] operator bool() const noexcept { return m_initialized; }

    [[nodiscard]] size_t getID() const noexcept { return m_id; }
    [[nodiscard]] Type   getType() const noexcept { return m_type; }

    const Presets& availablePresets() const noexcept { return m_presets; }
    const Preset&  currentPreset() const noexcept {
        if (m_preset_idx < m_presets.size()) {
            return m_presets[m_preset_idx];
        }
        return m_presets[0];
    }
    const size_t currentPresetIdx() const noexcept { return m_preset_idx; }

   protected:
    const size_t m_id;
    const Type   m_type;
    bool         m_initialized;
    size_t       m_preset_idx;
    Presets      m_presets;
};

}  // namespace ma

#endif