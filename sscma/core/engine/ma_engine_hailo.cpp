#include "ma_engine_hailo.h"

#if MA_USE_ENGINE_HAILO

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>

namespace ma::engine {

using namespace std;

EngineHailo::EngineHailo() : _vdevice(nullptr), _model(nullptr), _configured_model(nullptr), _bindings(nullptr) {}

EngineHailo::~EngineHailo() {}

ma_err_t EngineHailo::init() {
    if (_vdevice) {
        return MA_OK;
    }

    // TODO: optimize
    static auto get_vdevice_f = []() -> shared_ptr<VDevice> {
        static unique_ptr<VDevice> vdevice = nullptr;
        if (!vdevice) {
            auto dev = VDevice::create();
            if (!dev) {
                return nullptr;
            }
            vdevice = move(dev.release());
        }
        if (!vdevice) {
            return nullptr;
        }
        auto shared = vdevice.get();
        auto mgr = &vdevice;
        static atomic<size_t> ref_count = 0;
        ref_count.fetch_add(1);
        return shared_ptr<VDevice>(shared, [mgr](VDevice*) {
            if (mgr) {
                if (ref_count.fetch_sub(1) == 1) {
                    mgr->reset();
                }
            }
        });
    };
    _vdevice = get_vdevice_f();
    if (!_vdevice) {
        return MA_FAILED;
    }

    return MA_OK;
}

ma_err_t EngineHailo::init(size_t size) {
    return init();
}

ma_err_t EngineHailo::init(void* pool, size_t size) {
    return init();
}

ma_err_t EngineHailo::run() {
    if (!_configured_model || !_bindings) {
        return MA_FAILED;
    }

    using namespace std::chrono_literals;
    auto sta = _configured_model->wait_for_async_ready(1000ms);
    if (sta != HAILO_SUCCESS) {
        return MA_FAILED;
    }

    auto job = _configured_model->run_async(*_bindings, [&](const AsyncInferCompletionInfo& info) { sta = info.status; });

    do {
        this_thread::yield();
    } while (job->wait(50ms) != HAILO_SUCCESS);

    switch (sta) {
        case HAILO_SUCCESS:
            return MA_OK;
        case HAILO_TIMEOUT:
            return MA_ETIMEOUT;
        default:
            return MA_FAILED;
    }
}

#if MA_USE_FILESYSTEM
ma_err_t EngineHailo::load(const string& model_path) {

    {
        _input_tensors.clear();
        _output_tensors.clear();
        _io_buffers.clear();
        _external_handlers.clear();
    }

    {
        auto hef = Hef::create(model_path);
        if (!hef) {
            return MA_FAILED;
        }

        if (!_vdevice) {
            return MA_FAILED;
        }

        auto model = _vdevice->create_infer_model(model_path);
        if (!model) {
            return MA_FAILED;
        }
        _model = move(model.value());
        _model->set_hw_latency_measurement_flags(HAILO_LATENCY_MEASURE);

        auto configured_model = _model->configure();
        if (!configured_model) {
            return MA_FAILED;
        }
        {
            auto shared = new ConfiguredInferModel(configured_model.value());
            if (!shared) {
                return MA_ENOMEM;
            }

            _configured_model = shared_ptr<ConfiguredInferModel>(shared, [dep_ref = _model](ConfiguredInferModel* ptr) {
                delete ptr;
                ptr = nullptr;
                dep_ref.~shared_ptr();
            });
        }

        auto bindings = configured_model.value().create_bindings();
        if (!bindings) {
            return MA_FAILED;
        }
        {
            auto shared = new ConfiguredInferModel::Bindings(bindings.value());
            if (!shared) {
                return MA_ENOMEM;
            }

            _bindings = shared_ptr<ConfiguredInferModel::Bindings>(shared, [dep_ref = _configured_model](ConfiguredInferModel::Bindings* ptr) {
                delete ptr;
                ptr = nullptr;
                dep_ref.~shared_ptr();
            });
        }
    }

    {

        auto create_internal_bindings =
            [&](const string& name, const InferModel::InferStream& tsr, shared_ptr<ma_tensor_t>& tensor, hailort::ConfiguredInferModel::Bindings::InferStream* cis, bool is_input) -> ma_err_t {
            auto shape  = tsr.shape();
            auto size   = tsr.get_frame_size();
            auto format = tsr.format();

            if (!cis) {
                return MA_FAILED;
            }

            void* buffer = aligned_alloc(4096, size);
            if (!buffer) {
                return MA_ENOMEM;
            }

            tensor = shared_ptr<ma_tensor_t>(new ma_tensor_t{0}, [](ma_tensor_t* ptr) {
                if (ptr) {
                    if (ptr->data.data) {
                        free(ptr->data.data);
                        ptr->data.data = nullptr;
                    }
                    delete ptr;
                    ptr = nullptr;
                }
            });
            if (!tensor) {
                return MA_ENOMEM;
            }

            cis->set_buffer(MemoryView(buffer, size));

            tensor->data.data = buffer;
            tensor->size      = size;

            tensor->shape.size    = 4;
            tensor->shape.dims[0] = 1;
            switch (format.order) {
                case HAILO_FORMAT_ORDER_NCHW:
                    tensor->shape.dims[1] = shape.features;
                    tensor->shape.dims[2] = shape.height;
                    tensor->shape.dims[3] = shape.width;
                    break;
                case HAILO_FORMAT_ORDER_NHWC:
                case HAILO_FORMAT_ORDER_FCR:
                case HAILO_FORMAT_ORDER_HAILO_NMS:
                    tensor->shape.dims[1] = shape.height;
                    tensor->shape.dims[2] = shape.width;
                    tensor->shape.dims[3] = shape.features;
                    break;
                case HAILO_FORMAT_ORDER_NHCW:
                    tensor->shape.dims[1] = shape.height;
                    tensor->shape.dims[2] = shape.features;
                    tensor->shape.dims[3] = shape.width;
                    break;
                case HAILO_FORMAT_ORDER_NC:
                    if (shape.width != 1 || shape.height != 1) {
                        tensor->shape.dims[1] = shape.features;
                        tensor->shape.dims[2] = shape.height;
                        tensor->shape.dims[3] = shape.width;
                        break;
                    }
                    tensor->shape.dims[1] = shape.features;
                    tensor->shape.size    = 2;
                    break;
                default:
                    break;
            }

            auto quant_info = tsr.get_quant_infos();
            if (!quant_info.empty()) {
                tensor->quant_param.scale      = quant_info[0].qp_scale;
                tensor->quant_param.zero_point = quant_info[0].qp_zp;
            } else {
                tensor->quant_param.scale      = 1.0f;
                tensor->quant_param.zero_point = 0;
            }

            switch (format.type) {
                case HAILO_FORMAT_TYPE_AUTO:
                    tensor->type = MA_TENSOR_TYPE_NONE;
                    break;
                case HAILO_FORMAT_TYPE_UINT8:
                    tensor->type = MA_TENSOR_TYPE_U8;
                    break;
                case HAILO_FORMAT_TYPE_UINT16:
                    tensor->type = MA_TENSOR_TYPE_U16;
                    break;
                case HAILO_FORMAT_TYPE_FLOAT32:
                    tensor->type = MA_TENSOR_TYPE_F32;
                    break;
                default:
                    tensor->type = MA_TENSOR_TYPE_NONE;
                    break;
            }

            if (format.order == HAILO_FORMAT_ORDER_HAILO_NMS) {
                switch (format.type) {
                    case HAILO_FORMAT_TYPE_UINT16:
                        tensor->type = MA_TENSOR_TYPE_NMS_BBOX_U16;
                        break;
                    case HAILO_FORMAT_TYPE_FLOAT32:
                        tensor->type = MA_TENSOR_TYPE_NMS_BBOX_F32;
                        break;
                    default:
                        tensor->type = MA_TENSOR_TYPE_NONE;
                        break;
                }

                auto fp = make_shared<ExternalHandler>([this_ptr = this, name, is_input](int flag, void* data, size_t size) -> ma_err_t {
                    if (!data) {
                        return MA_EINVAL;
                    }
                    auto tsr = is_input ? this_ptr->_model->input(name) : this_ptr->_model->output(name);
                    if (!tsr) {
                        return MA_FAILED;
                    }
                    switch (flag) {
                        case 0:  // get score threshold
                            return MA_ENOTSUP;
                        case 1:  // set score threshold
                        {
                            if (sizeof(float) != size) {
                                return MA_EINVAL;
                            }
                            float threshold = *static_cast<float*>(data);
                            tsr->set_nms_score_threshold(threshold);
                            return MA_OK;
                        }
                        case 2:  // get iou threshold
                            return MA_ENOTSUP;
                        case 3:  // set iou threshold
                        {
                            if (sizeof(float) != size) {
                                return MA_EINVAL;
                            }
                            float threshold = *static_cast<float*>(data);
                            tsr->set_nms_iou_threshold(threshold);
                            return MA_OK;
                        }
                        case 4:  // get nms shape
                        {
                            auto nms_shape = tsr->get_nms_shape();
                            if (!nms_shape) {
                                return MA_FAILED;
                            }
                            auto shape = nms_shape.value();
                            if (sizeof(hailo_nms_shape_t) != size) {
                                return MA_EINVAL;
                            }
                            *static_cast<hailo_nms_shape_t*>(data) = shape;
                            return MA_OK;
                        }
                        default:
                            return MA_ENOTSUP;
                    }
                });

                _external_handlers[name] = fp;
                tensor->external_handler = reinterpret_cast<void*>(fp.get());
            }

            _io_buffers[name] = tensor;

            return MA_OK;
        };

        const auto& inputs = _model->inputs();
        for (const auto& tsr : inputs) {
            auto name = tsr.name();
            if (_io_buffers.find(name) != _io_buffers.end()) {
                continue;
            }
            shared_ptr<ma_tensor_t> tensor = nullptr;
            auto bindings_input            = _bindings->input(name);
            if (!bindings_input) {
                return MA_FAILED;
            }
            auto ret = create_internal_bindings(name, tsr, tensor, &bindings_input.value(), true);
            if (ret != MA_OK) {
                return ret;
            }
            _input_tensors.push_back(tensor);
        }

        const auto& outputs = _model->outputs();
        for (const auto& tsr : outputs) {
            auto name = tsr.name();
            if (_io_buffers.find(name) != _io_buffers.end()) {
                continue;
            }
            shared_ptr<ma_tensor_t> tensor = nullptr;
            auto bindings_output           = _bindings->output(name);
            if (!bindings_output) {
                return MA_FAILED;
            }
            auto ret = create_internal_bindings(name, tsr, tensor, &bindings_output.value(), false);
            if (ret != MA_OK) {
                return ret;
            }
            _output_tensors.push_back(tensor);
        }
    }

    return MA_OK;
}

ma_err_t EngineHailo::load(const char* model_path) {
    return load(string(model_path));
}
#endif

ma_err_t EngineHailo::load(const void* model_data, size_t model_size) {
#if MA_USE_FILESYSTEM
    string model_path(reinterpret_cast<const char*>(model_data), model_size);
    return load(model_path);
#else
    return MA_ENOTSUP;
#endif
}

int32_t EngineHailo::getInputSize() {
    return _input_tensors.size();
}

int32_t EngineHailo::getOutputSize() {
    return _output_tensors.size();
}

ma_tensor_t EngineHailo::getInput(int32_t index) {
    if (index < 0 || index >= static_cast<int32_t>(_input_tensors.size())) {
        return {0};
    }

    return _input_tensors[index] ? *_input_tensors[index] : ma_tensor_t{0};
}

ma_tensor_t EngineHailo::getOutput(int32_t index) {
    if (index < 0 || index >= static_cast<int32_t>(_output_tensors.size())) {
        return {0};
    }

    return _output_tensors[index] ? *_output_tensors[index] : ma_tensor_t{0};
}

ma_shape_t EngineHailo::getInputShape(int32_t index) {
    if (index < 0 || index >= static_cast<int32_t>(_input_tensors.size())) {
        return {0};
    }

    return _input_tensors[index] ? _input_tensors[index]->shape : ma_shape_t{0};
}

ma_shape_t EngineHailo::getOutputShape(int32_t index) {
    if (index < 0 || index >= static_cast<int32_t>(_output_tensors.size())) {
        return {0};
    }

    return _output_tensors[index] ? _output_tensors[index]->shape : ma_shape_t{0};
}

ma_quant_param_t EngineHailo::getInputQuantParam(int32_t index) {
    if (index < 0 || index >= static_cast<int32_t>(_input_tensors.size())) {
        return {0};
    }

    return _input_tensors[index] ? _input_tensors[index]->quant_param : ma_quant_param_t{0};
}

ma_quant_param_t EngineHailo::getOutputQuantParam(int32_t index) {
    if (index < 0 || index >= static_cast<int32_t>(_output_tensors.size())) {
        return {0};
    }

    return _output_tensors[index] ? _output_tensors[index]->quant_param : ma_quant_param_t{0};
}


ma_err_t EngineHailo::setInput(int32_t index, const ma_tensor_t& tensor) {
    if (index < 0 || index >= static_cast<int32_t>(_input_tensors.size())) {
        return MA_EINVAL;
    }

    if (tensor.size != _input_tensors[index]->size) {
        return MA_EINVAL;
    }

    std::memcpy(_input_tensors[index]->data.data, tensor.data.data, tensor.size);

    return MA_ENOTSUP;
}

}  // namespace ma::engine

#endif