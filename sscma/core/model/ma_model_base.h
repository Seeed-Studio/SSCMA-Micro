#ifndef _MA_MODEL_BASE_H_
#define _MA_MODEL_BASE_H_

#include <cstdarg>
#include <functional>
#include <string>

#include "../engine/ma_engine.h"
#include "../ma_common.h"

namespace ma {

using namespace ma::engine;
class Model {
private:
    ma_perf_t perf_;
    std::function<void(void*)> p_preprocess_done_;
    std::function<void(void*)> p_postprocess_done_;
    std::function<void(void*)> p_underlying_run_done_;
    void* p_user_ctx_;
    uint16_t m_type_;

protected:
    Engine* p_engine_;
    const char* p_name_;
    virtual ma_err_t preprocess()  = 0;
    virtual ma_err_t postprocess() = 0;
    ma_err_t underlyingRun();

public:
    Model(Engine* engine, const char* name, uint16_t type);
    virtual ~Model();
    const ma_perf_t getPerf() const;
    const char* getName() const;
    ma_model_type_t getType() const;
    ma_input_type_t getInputType() const;
    ma_output_type_t getOutputType() const;
    virtual const void* getInput()                          = 0;
    virtual ma_err_t setConfig(ma_model_cfg_opt_t opt, ...) = 0;
    virtual ma_err_t getConfig(ma_model_cfg_opt_t opt, ...) = 0;
    void setPreprocessDone(std::function<void(void*)> func);
    void setPostprocessDone(std::function<void(void*)> func);
    void setRunDone(std::function<void(void*)> func);
    void setUserCtx(void* ctx);
};
}  // namespace ma

#endif /* _MA_ALGO_H_ */
