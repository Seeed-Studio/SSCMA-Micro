#include "core/model/ma_model_base.h"

namespace ma::model {

const static char* TAG = "ma::model";


Model::Model(Engine* engine, const char* name) : p_engine_(engine), p_name_(name) {}


Model::~Model() {}


ma_err_t Model::underlying_run(const void* input) {

    ma_err_t err        = MA_OK;
    uint32_t start_time = 0;

    start_time = ma_get_time_ms();
    err        = preprocess(input);
    if (p_preprocess_done_ != nullptr) {
        p_preprocess_done_();
    }
    perf_.preprocess = ma_get_time_ms() - start_time;
    if (err != MA_OK) {
        return err;
    }
    start_time = ma_get_time_ms();


    start_time = ma_get_time_ms();
    err        = p_engine_->run();

    if (p_underlying_run_done_ != nullptr) {
        p_underlying_run_done_();
    }


    perf_.run = ma_get_time_ms() - start_time;

    if (err != MA_OK) {
        return err;
    }


    start_time = ma_get_time_ms();
    err        = postprocess();

    if (p_postprocess_done_ != nullptr) {
        p_postprocess_done_();
    }

    perf_.postprocess = ma_get_time_ms() - start_time;
    if (err != MA_OK) {
        return err;
    }

    return err;
}


ma_pref_t Model::get_perf() const {
    return perf_;
}


const char* Model::get_name() const {
    return p_name_;
}


void Model::set_preprocess_done(void (*fn)(void)) {
    if (p_preprocess_done_ != nullptr) {
        p_preprocess_done_ = nullptr;
        MA_LOGW(TAG, "preprocess done function already set, overwrite it");
    }
    p_preprocess_done_ = fn;
}


void Model::set_postprocess_done(void (*fn)(void)) {
    if (p_postprocess_done_ != nullptr) {
        p_postprocess_done_ = nullptr;
        MA_LOGW(TAG, "postprocess done function already set, overwrite it");
    }
    p_postprocess_done_ = fn;
}


void Model::set_run_done(void (*fn)(void)) {
    if (p_underlying_run_done_ != nullptr) {
        p_underlying_run_done_ = nullptr;
        MA_LOGW(TAG, "underlying run done function already set, overwrite it");
    }
    p_underlying_run_done_ = fn;
}


}  // namespace ma::model