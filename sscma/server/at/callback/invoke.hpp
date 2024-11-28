#pragma once

#include <ma_config_board.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/ma_core.h"
#include "core/utils/ma_base64.h"
#include "porting/ma_porting.h"
#include "refactor_required.hpp"
#include "resource.hpp"
#include "server/at/codec/ma_codec.h"

extern "C" {

#if MA_INVOKE_ENABLE_RUN_HOOK
extern void ma_invoke_pre_hook(void*);
extern void ma_invoke_post_hook(void*);
#endif
}

namespace ma::server::callback {

using namespace ma;

class Invoke final : public std::enable_shared_from_this<Invoke> {
public:
    std::shared_ptr<Invoke> getptr() {
        return shared_from_this();
    }

    [[nodiscard]] static std::shared_ptr<Invoke> create(const std::vector<std::string>& args, Transport& transport, Encoder& encoder, size_t task_id) {
        return std::shared_ptr<Invoke>{new Invoke{args, transport, encoder, task_id}};
    }

    ~Invoke() {
        if (_algorithm != nullptr) {
            ModelFactory::remove(_algorithm);
            _algorithm = nullptr;
        }

        static_resource->is_sample = false;
    }

    inline void run() {
        prepare();
    }

protected:
    Invoke(const std::vector<std::string>& args, Transport& transport, Encoder& encoder, size_t task_id) {
        MA_ASSERT(args.size() >= 2);
        _cmd          = args[0];
        _n_times      = std::atoi(args[1].c_str());
        _results_only = std::atoi(args[2].c_str());

        _ret = MA_OK;

        _sensor    = nullptr;
        _transport = &transport;
        _encoder   = &encoder;
        _model     = ma_model_t{};
        _algorithm = nullptr;

        _times = 0;

        _task_id = task_id;

        _preprocess_hook_injected = false;

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

        if (!prepareModel()) [[unlikely]]
            goto Err;

        if (!prepareAlgorithm()) [[unlikely]]
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

    bool prepareModel() {
        const auto& models = static_resource->device->getModels();
        auto it            = std::find_if(models.begin(), models.end(), [&](const ma_model_t& m) { return m.id == static_resource->current_model_id; });
        if (it == models.end()) {
            _ret = MA_ENOENT;
        } else if (it->addr == nullptr) {
            _ret = MA_ENOENT;
        } else {
            _model = *it;
        }
        return isEverythingOk();
    }

    bool prepareAlgorithm() {
#if MA_USE_FILESYSTEM
        _ret = static_resource->engine->load(static_cast<const char*>(_model.addr));
#else
        _ret = static_resource->engine->load(_model.addr, _model.size);
#endif
        if (!isEverythingOk()) {
            return false;
        }
        _algorithm = ModelFactory::create(static_resource->engine, static_resource->current_model_id);
        if (_algorithm == nullptr) {
            _ret = MA_ENOTSUP;
        }
#if MA_INVOKE_ENABLE_RUN_HOOK
        _algorithm->setRunDone([](void*) { ma_invoke_post_hook(nullptr); });
#endif
        return isEverythingOk();
    }

    void directReply() {
        _encoder->begin(MA_MSG_TYPE_RESP, _ret, _cmd);
        _encoder->write(_sensor, _sensor->currentPresetIdx());
        std::vector<ma_model_t> model{_model};
        _encoder->write(model);
        _encoder->write(_algorithm ? static_cast<int>(_algorithm->getType()) : static_cast<int>(static_resource->current_algorithm_id),
                        0,
                        _sensor ? static_cast<int>(_sensor->getType()) : 0,
                        static_cast<int>(static_resource->shared_threshold_score * 100.0),
                        static_cast<int>(static_resource->shared_threshold_nms * 100.0));
        _encoder->end();
        _transport->send(reinterpret_cast<const char*>(_encoder->data()), _encoder->size());
    }


    void eventReply(int width, int height) {

        _encoder->begin(MA_MSG_TYPE_EVT, _ret, _cmd);
        _encoder->write("count", _times);

        if (!_results_only) {
#if MA_SENSOR_ENCODE_USE_STATIC_BUFFER
            reinterpret_cast<char*>(_buffer)[_buffer_size] = '\0';
            _encoder->write("image", reinterpret_cast<const char*>(_buffer), _buffer_size);
#else
            _encoder->write("image", _buffer);
#endif
        }

        serializeAlgorithmOutput(_algorithm, _encoder, width, height);

        auto perf = _algorithm->getPerf();
        _encoder->write(perf);
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

        MA_LOGD(MA_TAG, "eventLoopCamera");

        auto camera     = static_cast<Camera*>(_sensor);
        auto frame      = ma_img_t{};
        auto raw_frame  = ma_img_t{};
        int buffer_size = 0;

        _ret = camera->retrieveFrame(raw_frame, MA_PIXEL_FORMAT_AUTO);
        if (!isEverythingOk()) [[unlikely]]
            goto Err;
        if (!_results_only) {
            _ret = camera->retrieveFrame(frame, MA_PIXEL_FORMAT_JPEG);
            if (!isEverythingOk()) [[unlikely]]
                goto Err;

            buffer_size = 4 * ((frame.size + 2) / 3);
#if MA_SENSOR_ENCODE_USE_STATIC_BUFFER
            if (buffer_size > MA_SENSOR_ENCODE_STATIC_BUFFER_SIZE) {
                MA_LOGE(MA_TAG, "buffer_size > MA_SENSOR_ENCODE_STATIC_BUFFER_SIZE");
                goto Err;
            }

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
                _buffer_size                             = buffer_size;
                static_cast<char*>(_buffer)[buffer_size] = '\0';
#else
                _buffer[buffer_size] = '\0';
#endif
                if (ret != MA_OK) {
                    MA_LOGE(MA_TAG, "base64_encode failed: %d", ret);
                }
            }

            camera->returnFrame(frame);
        }
        if (!_preprocess_hook_injected) {
            _preprocess_hook_injected = true;
            _algorithm->setPreprocessDone([camera, &raw_frame](void*) {
                camera->returnFrame(raw_frame);
#if MA_INVOKE_ENABLE_RUN_HOOK
                ma_invoke_pre_hook(nullptr);
#endif
            });
        }

        if (!_event_hook) {
            _event_hook = [&raw_frame](Encoder& encoder) {
                int16_t rotation = static_cast<int>(raw_frame.rotate) * 90;
                encoder.write("rotation", rotation);
                encoder.write("width", raw_frame.width);
                encoder.write("height", raw_frame.height);
            };
        }

        // update configs TODO: refactor
        _algorithm->setConfig(MA_MODEL_CFG_OPT_THRESHOLD, static_resource->shared_threshold_score);
        _algorithm->setConfig(MA_MODEL_CFG_OPT_NMS, static_resource->shared_threshold_nms);

        _ret = setAlgorithmInput(_algorithm, raw_frame);
        if (!isEverythingOk()) [[unlikely]]
            goto Err;

        {
            trigger_rules_mutex.lock();
            auto trigger_copy = trigger_rules;
            trigger_rules_mutex.unlock();
            for (auto& rule : trigger_copy) {
                if (rule) {
                    (*rule.get())(_algorithm);
                }
            }
        }

        eventReply(raw_frame.width, raw_frame.height);

        static_resource->executor->submit([_this = std::move(getptr())](const std::atomic<bool>&) { _this->eventLoopCamera(); });
        return;

Err:
        eventReply(raw_frame.width, raw_frame.height);
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
    ma_model_t _model;
    Model* _algorithm;

    size_t _task_id;
    int32_t _times;
    bool _results_only;

    bool _preprocess_hook_injected;
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
