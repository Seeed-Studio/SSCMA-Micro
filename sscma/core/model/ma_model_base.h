#ifndef _MA_MODEL_BASE_H_
#define _MA_MODEL_BASE_H_

#include "core/ma_common.h"

#include "core/engine/ma_engine.h"

namespace ma::model {

class Model {
private:
    ma_pref_t   perf_;
    const char* p_name_;
    void (*p_preprocess_done_)(void);
    void (*p_postprocess_done_)(void);
    void (*p_underlying_run_done_)(void);

protected:
    Engine*          p_engine_;
    virtual bool     is_valid()                    = 0;
    virtual ma_err_t preprocess(const void* input) = 0;
    virtual ma_err_t postprocess()                 = 0;
    virtual ma_err_t underlying_run(const void* input);

public:
    Model(Engine* engine, const char* name);
    virtual ~Model();
    virtual ma_err_t run(const void* input) = 0;
    ma_pref_t        get_perf() const;
    const char*      get_name() const;
    void             set_preprocess_done(void (*fn)(void));
    void             set_postprocess_done(void (*fn)(void));
    void             set_run_done(void (*fn)(void));
};

}  // namespace ma::model

#endif /* _MA_ALGO_H_ */
