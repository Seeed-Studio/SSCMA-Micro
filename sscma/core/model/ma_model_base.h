#ifndef _MA_MODEL_BASE_H_
#define _MA_MODEL_BASE_H_

#include <forward_list>

#include "core/ma_common.h"

#include "core/engine/ma_engine.h"

namespace ma::model {

template <typename Result>
class Model {
private:
    ma_pref_t       perf_;
    ma_model_type_t type_;
    const char*     p_name_;

protected:
    Engine*                   p_engine_;
    std::forward_list<Result> results_;
    virtual ma_err_t          preprocess()                                                     = 0;
    virtual ma_err_t          postprocess()                                                    = 0;
    virtual ma_err_t          underlying_run(ma_tensor_t input[], size_t input_size = 1)       = 0;
    virtual ma_err_t          underlying_async_run(ma_tensor_t input[], size_t input_size = 1) = 0;

public:
    Model(Engine* engine, const char* name);
    virtual ~Model();
    virtual ma_err_t           init()   = 0;
    virtual ma_err_t           deinit() = 0;
    ma_err_t                   run(ma_tensor_t input[], size_t input_size = 1);
    ma_err_t                   async_run(ma_tensor_t input[], size_t input_size = 1);
    bool                       wait();
    std::forward_list<Result>& get_results();
    ma_pref_t                  get_pref() const;
    const char*                get_name() const;
};

}  // namespace ma::model

#endif /* _MA_ALGO_H_ */
