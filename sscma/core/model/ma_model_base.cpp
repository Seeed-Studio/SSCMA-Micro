#include "core/model/ma_model_base.h"

namespace ma::model {

constexpr char TAG[] = "ma::model";

Model::Model(Engine* engine, const char* name, ma_model_type_t type)
    : p_engine_(engine), p_name_(name), m_type_(type) {
    p_user_ctx_            = nullptr;
    p_preprocess_done_     = nullptr;
    p_postprocess_done_    = nullptr;
    p_underlying_run_done_ = nullptr;
    perf_.preprocess       = 0;
    perf_.inference        = 0;
    perf_.postprocess      = 0;
}

Model::~Model() {}

ma_err_t Model::underlyingRun() {

    ma_err_t err       = MA_OK;
    int64_t start_time = 0;

    start_time = ma_get_time_ms();

    err = preprocess();
    if (p_preprocess_done_ != nullptr) {
        p_preprocess_done_(p_user_ctx_);
    }
    perf_.preprocess = ma_get_time_ms() - start_time;
    if (err != MA_OK) {
        return err;
    }


    start_time = ma_get_time_ms();
    err        = p_engine_->run();
    if (p_underlying_run_done_ != nullptr) {
        p_underlying_run_done_(p_user_ctx_);
    }

    perf_.inference = ma_get_time_ms() - start_time;


    if (err != MA_OK) {
        return err;
    }
    start_time = ma_get_time_ms();
    err        = postprocess();
    if (p_postprocess_done_ != nullptr) {
        p_postprocess_done_(p_user_ctx_);
    }
    perf_.postprocess = ma_get_time_ms() - start_time;

    if (err != MA_OK) {
        return err;
    }

    return err;
}


const ma_perf_t Model::getPerf() const {
    return perf_;
}


const char* Model::getName() const {
    return p_name_;
}

void Model::setPreprocessDone(void (*fn)(void* ctx)) {
    if (p_preprocess_done_ != nullptr) {
        p_preprocess_done_ = nullptr;
        MA_LOGW(TAG, "preprocess done function already set, overwrite it");
    }
    p_preprocess_done_ = fn;
}

void Model::setPostprocessDone(void (*fn)(void* ctx)) {
    if (p_postprocess_done_ != nullptr) {
        p_postprocess_done_ = nullptr;
        MA_LOGW(TAG, "postprocess done function already set, overwrite it");
    }
    p_postprocess_done_ = fn;
}

void Model::setRunDone(void (*fn)(void* ctx)) {
    if (p_underlying_run_done_ != nullptr) {
        p_underlying_run_done_ = nullptr;
        MA_LOGW(TAG, "underlying run done function already set, overwrite it");
    }
    p_underlying_run_done_ = fn;
}

void Model::setUserCtx(void* ctx) {
    if (p_user_ctx_ != nullptr) {
        p_user_ctx_ = nullptr;
        MA_LOGW(TAG, "user ctx already set, overwrite it");
    }
    p_user_ctx_ = ctx;
}

}  // namespace ma