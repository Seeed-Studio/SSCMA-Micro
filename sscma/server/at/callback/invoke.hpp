#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/ma_core.h"
#include "core/utils/ma_base64.h"
#include "ma_codec.h"
#include "porting/ma_porting.h"
#include "refactor_required.hpp"
#include "resource.hpp"

static std::function<void()> _preprocess_done;
static void                  _on_preprocess_done(void*) {
    if (_preprocess_done) _preprocess_done();
}

namespace ma::server::callback {

using namespace ma;

class Invoke final : public std::enable_shared_from_this<Invoke> {
   public:
    std::shared_ptr<Invoke> getptr() { return shared_from_this(); }

    [[nodiscard]] static std::shared_ptr<Invoke> create(const std::vector<std::string>& args,
                                                        Transport&                      transport,
                                                        Encoder&                        encoder,
                                                        size_t                          task_id) {
        return std::shared_ptr<Invoke>{
          new Invoke{args, transport, encoder, task_id}
        };
    }

    ~Invoke() {
        if (_algorithm != nullptr) {
            ModelFactory::remove(_algorithm);
            _algorithm = nullptr;
        }

        static_resource->is_sample = false;
    }

    inline void run() { prepare(); }

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
            return static_resource->executor->submit(
              [_this = std::move(getptr())](const std::atomic<bool>&) { _this->eventLoopCamera(); });
        default:
            _ret = MA_ENOTSUP;
            directReply();
        }

    Err:
        directReply();
    }

    bool prepareSensor() {
        const auto& sensors = static_resource->device->getSensors();
        auto        it      = std::find_if(sensors.begin(), sensors.end(), [&](const Sensor* s) {
            return s->getID() == static_resource->current_sensor_id;
        });
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
        auto        it     = std::find_if(
          models.begin(), models.end(), [&](const ma_model_t& m) { return m.id == static_resource->current_model_id; });
        if (it == models.end()) {
            _ret = MA_ENOENT;
        }
        _model = *it;
        return isEverythingOk();
    }

    bool prepareAlgorithm() {
#if MA_USE_FILESYSTEM
        _ret = static_resource->engine->load(_model.addr);
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
        return isEverythingOk();
    }

    void directReply() {
        _encoder->begin(MA_MSG_TYPE_RESP, _ret, _cmd);
        _encoder->write(_sensor, _sensor->currentPresetIdx());
        _encoder->end();
        _transport->send(reinterpret_cast<const char*>(_encoder->data()), _encoder->size());
    }

    void eventReply(int32_t w, int32_t h) {
        _encoder->begin(MA_MSG_TYPE_EVT, _ret, _cmd);
        _encoder->write("count", _times);
        if (!_results_only) _encoder->write("image", _buffer);
        serializeAlgorithmOutput(_algorithm, _encoder);
        _encoder->write("width", w);
        _encoder->write("height", h);
        _encoder->end();
        _transport->send(reinterpret_cast<const char*>(_encoder->data()), _encoder->size());
    }

    void eventLoopCamera() {
        
        if ((_n_times >= 0) & (_times++ >= _n_times)) [[unlikely]]
            return;
        if (static_resource->current_task_id.load() != _task_id) [[unlikely]]
            return;

        MA_LOGD(MA_TAG, "eventLoopCamera");

        auto camera      = static_cast<Camera*>(_sensor);
        auto frame       = ma_img_t{};
        auto raw_frame   = ma_img_t{};
        int  buffer_size = 0;

        _ret = camera->retrieveFrame(raw_frame, MA_PIXEL_FORMAT_AUTO);
        if (!isEverythingOk()) [[unlikely]]
            goto Err;
        if (!_results_only) {
            _ret = camera->retrieveFrame(frame, MA_PIXEL_FORMAT_JPEG);
            if (!isEverythingOk()) [[unlikely]]
                goto Err;

            buffer_size = 4 * ((frame.size + 2) / 3);
            _buffer.resize(buffer_size);

            ma::utils::base64_encode(reinterpret_cast<unsigned char*>(frame.data),
                                     frame.size,
                                     reinterpret_cast<char*>(_buffer.data()),
                                     &buffer_size);

            camera->returnFrame(frame);
        }
        if (!_preprocess_done) {
            _preprocess_done = [camera, &raw_frame]() { camera->returnFrame(raw_frame); };
            _algorithm->setPreprocessDone(&_on_preprocess_done);
        }

        // update configs TODO: refactor
        _algorithm->setConfig(MA_MODEL_CFG_OPT_THRESHOLD, static_resource->shared_threshold_score);
        _algorithm->setConfig(MA_MODEL_CFG_OPT_NMS, static_resource->shared_threshold_nms);

        _ret = setAlgorithmInput(_algorithm, raw_frame);
        if (!isEverythingOk()) [[unlikely]]
            goto Err;

        eventReply(raw_frame.width, raw_frame.height);

        static_resource->executor->submit(
          [_this = std::move(getptr())](const std::atomic<bool>&) { _this->eventLoopCamera(); });
        return;

    Err:
        eventReply(raw_frame.width, raw_frame.height);
    }

    inline bool isEverythingOk() const { return _ret == MA_OK; }

   private:
    std::string _cmd;
    int32_t     _n_times;

    ma_err_t _ret;

    Sensor*    _sensor;
    Transport* _transport;
    Encoder*   _encoder;
    ma_model_t _model;
    Model*     _algorithm;

    size_t  _task_id;
    int32_t _times;
    bool    _results_only;

    std::string _buffer;
};

}  // namespace ma::server::callback
