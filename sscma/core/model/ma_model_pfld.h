#ifndef _MA_MODEL_PFLD_
#define _MA_MODEL_PFLD_

#include "ma_model_point_detector.h"

namespace ma::model {

class PFLD : public PointDetector {
   protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessI8();

   public:
    PFLD(Engine* engine);
    ~PFLD();

    static bool isValid(Engine* engine);

    static const char* getTag();
};

}  // namespace ma::model

#endif
