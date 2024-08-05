#ifndef _MA_MODEL_IMCLS_H
#define _MA_MODEL_IMCLS_H

#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class ImCls : public Detector {
private:
    ma_tensor_t output_;
   
protected:
    ma_err_t postprocess() override;

public:
    ImCls(Engine* engine);
    ~ImCls();
    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif  // _MA_MODEL_YOLO_H
