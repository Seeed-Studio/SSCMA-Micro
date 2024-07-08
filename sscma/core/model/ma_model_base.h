#ifndef _MA_MODEL_BASE_H_
#define _MA_MODEL_BASE_H_

#include "core/engine/ma_engine.h"
#include "core/ma_common.h"

namespace ma::model {

using namespace ma::engine;

class Model {
private:
    ma_perf_t perf_;
    void (*p_preprocess_done_)(void* ctx);
    void (*p_postprocess_done_)(void* ctx);
    void (*p_underlying_run_done_)(void* ctx);
    void* p_user_ctx_;
    ma_model_type_t m_type_;

protected:
    Engine* p_engine_;
    const char* p_name_;
    virtual ma_err_t preprocess()  = 0;
    virtual ma_err_t postprocess() = 0;
    ma_err_t underlyingRun();

public:
    Model(Engine* engine, const char* name, ma_model_type_t type);
    virtual ~Model();
    virtual bool isValid(Engine* engine) = 0;
    const ma_perf_t getPerf() const;
    const char* getName() const;
    ma_model_type_t getType() const;
    virtual ma_err_t setConfig(ma_model_cfg_opt_t opt, ...) = 0;
    virtual ma_err_t getConfig(ma_model_cfg_opt_t opt, ...) = 0;
    void setPreprocessDone(void (*fn)(void* ctx));
    void setPostprocessDone(void (*fn)(void* ctx));
    void setRunDone(void (*fn)(void* ctx));
    void setUserCtx(void* ctx);
};


}  // namespace ma::model

#endif /* _MA_ALGO_H_ */
