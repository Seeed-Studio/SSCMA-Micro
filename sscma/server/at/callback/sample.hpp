#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/ma_core.h"
#include "core/utils/ma_base64.h"
#include "porting/ma_porting.h"
#include "resource.hpp"
#include "server/at/codec/ma_codec.h"

namespace ma::server::callback {

using namespace ma;

class Sample final : public std::enable_shared_from_this<Sample> {
public:
    std::shared_ptr<Sample> getptr() {
        return shared_from_this();
    }

    [[nodiscard]] static std::shared_ptr<Sample> create(const std::vector<std::string>& args, Transport& transport, Encoder& encoder, size_t task_id) {
        return std::shared_ptr<Sample>{new Sample{args, transport, encoder, task_id}};
    }

    ~Sample() {
        static_resource->is_sample = false;
    }

    inline void run() {
        prepare();
    }

protected:
    Sample(const std::vector<std::string>& args, Transport& transport, Encoder& encoder, size_t task_id) {
        MA_ASSERT(args.size() >= 2);
        _cmd     = args[0];
        _n_times = std::atoi(args[1].c_str());

        _ret = MA_OK;

        _sensor    = nullptr;
        _transport = &transport;
        _encoder   = &encoder;

        _times = 0;

        _task_id = task_id;

#if MA_SENSOR_ENCODE_USE_STATIC_BUFFER
        _buffer      = reinterpret_cast<void*>(MA_SENSOR_ENCODE_STATIC_BUFFER_ADDR);
        _buffer_size = 0;
#endif

        static_resource->is_sample = true;
    }

private:
    void prepare() {
        if (!prepareSensor()) [[unlikely]]
            goto Err;

        switch (_sensor->getType()) {
            case Sensor::Type::kCamera:
                directReply();
                return static_resource->executor->submit([_this = std::move(getptr())](const std::atomic<bool>&) { _this->eventLoopCamera(); });
            default:
                _ret = MA_ENOTSUP;
                directReply();
        }

Err:
        directReply();
    }

    bool prepareSensor() {
        const auto& sensors = static_resource->device->getSensors();
        auto it             = std::find_if(sensors.begin(), sensors.end(), [&](const Sensor* s) { return s->getID() == static_resource->current_sensor_id; });
        if (it == sensors.end()) {
            return false;
        }
        _sensor      = *it;
        auto* camera = static_cast<Camera*>(_sensor);
        _ret         = camera->startStream(Camera::StreamMode::kRefreshOnReturn);
        return isEverythingOk();
    }

    void directReply() {
        _encoder->begin(MA_MSG_TYPE_RESP, _ret, _cmd);
        _encoder->write(_sensor, _sensor->currentPresetIdx());
        _encoder->end();
        _transport->send(reinterpret_cast<const char*>(_encoder->data()), _encoder->size());
    }

    void eventReply() {
        _encoder->begin(MA_MSG_TYPE_EVT, _ret, _cmd);
        _encoder->write("count", _times);
#if MA_SENSOR_ENCODE_USE_STATIC_BUFFER
        reinterpret_cast<char*>(_buffer)[_buffer_size] = '\0';
        _encoder->write("image", reinterpret_cast<char*>(_buffer), _buffer_size);
#else
        _encoder->write("image", _buffer);
#endif
        if (_event_hook)
            _event_hook(*_encoder);
        _encoder->end();
        _transport->send(reinterpret_cast<const char*>(_encoder->data()), _encoder->size());
    }

    void eventLoopCamera() {
        if ((_n_times >= 0) & (_times++ >= _n_times)) [[unlikely]]
            return;
        if (static_resource->current_task_id.load() != _task_id) [[unlikely]]
            return;

        auto camera     = static_cast<Camera*>(_sensor);
        auto frame      = ma_img_t{};
        int buffer_size = 0;

        _ret = camera->retrieveFrame(frame, MA_PIXEL_FORMAT_JPEG);
        if (!isEverythingOk()) [[unlikely]]
            goto Err;

        buffer_size = 4 * ((frame.size + 2) / 3);
#if MA_SENSOR_ENCODE_USE_STATIC_BUFFER
        if (buffer_size > MA_SENSOR_ENCODE_STATIC_BUFFER_SIZE) {
            MA_LOGE(MA_TAG, "buffer_size > MA_SENSOR_ENCODE_STATIC_BUFFER_SIZE");
            goto Err;
        }
        _buffer_size = buffer_size;
#else
        if (buffer_size > _buffer.size()) {
            _buffer.resize(buffer_size + 1);
        }
#endif

        {
            auto ret = ma::utils::base64_encode(reinterpret_cast<unsigned char*>(frame.data),
                                                frame.size,
#if MA_SENSOR_ENCODE_USE_STATIC_BUFFER
                                                reinterpret_cast<char*>(_buffer),
#else
                                                reinterpret_cast<char*>(_buffer.data()),
#endif
                                                &buffer_size);
#if MA_SENSOR_ENCODE_USE_STATIC_BUFFER
            static_cast<char*>(_buffer)[buffer_size] = '\0';
#else
            _buffer[buffer_size] = '\0';
#endif
            if (ret != MA_OK) {
                MA_LOGE(MA_TAG, "base64_encode failed: %d", ret);
            }
        }

        camera->returnFrame(frame);

        if (!_event_hook) {
            _event_hook = [&frame](Encoder& encoder) {
                int16_t rotation = static_cast<int>(frame.rotate) * 90;
                encoder.write("rotation", rotation);
                encoder.write("width", frame.width);
                encoder.write("height", frame.height);
            };
        }

        eventReply();

        static_resource->executor->submit([_this = std::move(getptr())](const std::atomic<bool>&) { _this->eventLoopCamera(); });
        return;

Err:
        eventReply();
    }

    inline bool isEverythingOk() const {
        return _ret == MA_OK;
    }

private:
    std::string _cmd;
    int32_t _n_times;

    ma_err_t _ret;

    Sensor* _sensor;
    Transport* _transport;
    Encoder* _encoder;

    size_t _task_id;
    int32_t _times;

    std::function<void(Encoder&)> _event_hook;

#if MA_SENSOR_ENCODE_USE_STATIC_BUFFER
#ifndef MA_SENSOR_ENCODE_STATIC_BUFFER_ADDR
#error "MA_SENSOR_ENCODE_STATIC_BUFFER_ADDR is not defined"
#endif
#ifndef MA_SENSOR_ENCODE_STATIC_BUFFER_SIZE
#error "MA_SENSOR_ENCODE_STATIC_BUFFER_SIZE is not defined"
#endif
    void* _buffer;
    size_t _buffer_size;
#else
    std::string _buffer;
#endif
};

}  // namespace ma::server::callback
